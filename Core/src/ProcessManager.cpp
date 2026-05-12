#include "pch.h"
#include "ProcessManager.h"

#include <TlHelp32.h>      // Toolhelp32 snapshot APIs
#include <Psapi.h>         // GetProcessMemoryInfo
#include <WtsApi32.h>      // (dự phòng cho phase sau)

#pragma comment(lib, "Psapi.lib")
// TlHelp32 nằm trong kernel32 nên không cần linker hint.

namespace cheatvn {

// =====================================================================
//  enableDebugPrivilege
// =====================================================================
//
// Windows phân chia quyền theo "privileges" (đặc quyền). SeDebugPrivilege
// cho phép process này mở handle tới mọi process khác (kể cả system) với
// quyền cao như PROCESS_VM_READ. Mặc định, ngay cả Administrator cũng KHÔNG
// auto-bật privilege này — phải tự gọi AdjustTokenPrivileges.
//
// Quy trình:
//   1) Lấy access token của process hiện tại (OpenProcessToken)
//   2) Tra cứu LUID của privilege "SeDebugPrivilege" (LookupPrivilegeValue)
//   3) Set TOKEN_PRIVILEGES.Attributes = SE_PRIVILEGE_ENABLED rồi gọi
//      AdjustTokenPrivileges để áp dụng
//
// Nếu user không chạy with Administrator thì AdjustTokenPrivileges sẽ
// trả về TRUE nhưng GetLastError() = ERROR_NOT_ALL_ASSIGNED — privilege
// bị từ chối. Đó là lý do phải check cả 2.
bool ProcessManager::enableDebugPrivilege() {
    HANDLE hToken = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(),
                          TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                          &hToken)) {
        return false;
    }

    LUID luid{};
    if (!LookupPrivilegeValueW(nullptr, SE_DEBUG_NAME, &luid)) {
        CloseHandle(hToken);
        return false;
    }

    TOKEN_PRIVILEGES tp{};
    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    BOOL ok = AdjustTokenPrivileges(hToken, FALSE, &tp, 0, nullptr, nullptr);
    DWORD err = GetLastError();   // Phải đọc NGAY trước khi gọi API khác
    CloseHandle(hToken);

    return ok && err != ERROR_NOT_ALL_ASSIGNED;
}

// =====================================================================
//  enumerateProcesses
// =====================================================================
//
// Toolhelp32 là API dễ dùng nhất để liệt kê process. Quy trình:
//   1) CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0)
//      → chụp ảnh trạng thái process table tại thời điểm gọi
//   2) Process32FirstW + vòng lặp Process32NextW → duyệt các entry
//   3) Với mỗi entry, mở handle nhẹ (PROCESS_QUERY_LIMITED_INFORMATION) để:
//        - lấy full path (QueryFullProcessImageNameW)
//        - lấy memory info (GetProcessMemoryInfo)
//        - phát hiện bitness (IsWow64Process)
//      Mở quyền THẤP để không bị từ chối ở các process bảo vệ.
//
// Lưu ý PROCESSENTRY32W (phiên bản W cho UTF-16). Phải set dwSize TRƯỚC
// khi gọi Process32FirstW — đây là pattern Win32 cũ, nếu quên sẽ fail.
std::vector<ProcessInfo> ProcessManager::enumerateProcesses() {
    std::vector<ProcessInfo> results;
    results.reserve(256);  // Hệ thống Windows trung bình có ~150-300 process.

    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap == INVALID_HANDLE_VALUE) {
        return results;
    }

    PROCESSENTRY32W entry{};
    entry.dwSize = sizeof(entry);     // BẮT BUỘC, không thì Process32FirstW fail

    if (!Process32FirstW(hSnap, &entry)) {
        CloseHandle(hSnap);
        return results;
    }

    do {
        // Bỏ qua PID 0 (System Idle Process) — không phải process thực.
        if (entry.th32ProcessID == 0) continue;

        ProcessInfo info;
        info.pid  = entry.th32ProcessID;
        info.name = entry.szExeFile;

        // Mở handle quyền tối thiểu để lấy thêm info.
        // PROCESS_QUERY_LIMITED_INFORMATION (Vista+) không cần SeDebugPrivilege
        // và hoạt động với cả nhiều process bảo vệ.
        HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION,
                                   FALSE,
                                   info.pid);
        if (hProc) {
            // --- Full path ---
            wchar_t pathBuf[MAX_PATH] = {0};
            DWORD pathLen = MAX_PATH;
            if (QueryFullProcessImageNameW(hProc, 0, pathBuf, &pathLen)) {
                info.path.assign(pathBuf, pathLen);
            }

            // --- Bitness ---
            // IsWow64Process trả về TRUE nếu process là 32-bit chạy
            // trên Windows 64-bit (qua lớp giả lập WoW64).
            // Process 64-bit thật → FALSE. (Trên Windows 32-bit thì luôn FALSE.)
            BOOL isWow64 = FALSE;
            IsWow64Process(hProc, &isWow64);
            info.is64Bit = !isWow64;

            // --- Memory usage ---
            PROCESS_MEMORY_COUNTERS pmc{};
            if (GetProcessMemoryInfo(hProc, &pmc, sizeof(pmc))) {
                // WorkingSetSize tính bằng bytes. Chuyển sang KB cho gọn.
                info.workingSetKB = pmc.WorkingSetSize / 1024;
            }

            info.isAccessible = true;
            CloseHandle(hProc);
        } else {
            // Open thất bại → process bảo vệ hoặc đã chết.
            // Vẫn add vào list nhưng đánh dấu inaccessible.
            info.isAccessible = false;
        }

        results.push_back(std::move(info));

    } while (Process32NextW(hSnap, &entry));

    CloseHandle(hSnap);
    return results;
}

// =====================================================================
//  openForReadWrite
// =====================================================================
//
// Mở handle với đầy đủ quyền để scanner hoạt động:
//   - PROCESS_VM_READ        → ReadProcessMemory
//   - PROCESS_VM_WRITE       → WriteProcessMemory
//   - PROCESS_VM_OPERATION   → VirtualProtectEx (khi page bị NO_ACCESS,
//                              ta phải tạm đổi protect để đọc)
//   - PROCESS_QUERY_INFORMATION → VirtualQueryEx, GetModuleInformation,...
//
// bInheritHandle = FALSE: child process (nếu app này spawn process khác)
// sẽ không kế thừa handle này — bảo mật hơn.
HANDLE ProcessManager::openForReadWrite(uint32_t pid) {
    DWORD desired = PROCESS_VM_READ
                  | PROCESS_VM_WRITE
                  | PROCESS_VM_OPERATION
                  | PROCESS_QUERY_INFORMATION;
    return OpenProcess(desired, FALSE, pid);
}

void ProcessManager::closeHandle(HANDLE h) {
    if (h && h != INVALID_HANDLE_VALUE) {
        CloseHandle(h);
    }
}

bool ProcessManager::isProcessAlive(HANDLE h) {
    if (!h) return false;
    DWORD exitCode = 0;
    if (!GetExitCodeProcess(h, &exitCode)) return false;
    // STILL_ACTIVE = 259. Nếu thực ra process đã exit với code 259,
    // ta vẫn nhận là "alive" — rất hiếm, chấp nhận được.
    return exitCode == STILL_ACTIVE;
}

} // namespace cheatvn

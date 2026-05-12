#pragma once

#include "ProcessInfo.h"
#include <vector>
#include <Windows.h>

namespace cheatvn {

// Quản lý việc liệt kê và mở handle tới các process trên hệ thống.
// Thread-safe ở mức API call (mỗi method là một transaction độc lập).
class ProcessManager {
public:
    // Bật SeDebugPrivilege cho process hiện tại.
    // Cần cho việc OpenProcess vào các process của user khác hoặc system.
    // Trả về true nếu thành công, false nếu thiếu quyền (chưa chạy Administrator).
    // Gọi 1 lần khi app khởi động.
    static bool enableDebugPrivilege();

    // Liệt kê toàn bộ process đang chạy.
    // Sử dụng Toolhelp32 snapshot — đơn giản, có sẵn tên process.
    // Trả về vector rỗng nếu fail (rất hiếm).
    static std::vector<ProcessInfo> enumerateProcesses();

    // Mở handle vào 1 process để đọc/ghi memory.
    // Trả về HANDLE hợp lệ, hoặc nullptr nếu fail.
    // Caller PHẢI gọi CloseHandle khi xong.
    static HANDLE openForReadWrite(uint32_t pid);

    // Đóng handle. Wrapper an toàn (handle = nullptr không crash).
    static void closeHandle(HANDLE h);

    // Kiểm tra process còn sống không (bằng cách check exit code).
    static bool isProcessAlive(HANDLE h);
};

} // namespace cheatvn

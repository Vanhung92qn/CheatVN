# 03 — Giải thích từng dòng code

> Mỗi file đã viết được giải thích **mục đích + từng phần code**.

---

## A. Cấu trúc thư mục hiện tại

```
CheatVN/                      ← gốc solution
├── CheatVN.sln                ← Solution (text file VS dùng để mở project)
├── .gitignore                 ← Git bỏ qua file build output
│
├── Core/                      ← Project native C++
│   ├── Core.vcxproj           ← Khai báo build cho Core
│   ├── Core.vcxproj.filters   ← Khai báo cây thư mục ảo trong VS
│   ├── pch.h, pch.cpp         ← Precompiled header (speed up compile)
│   ├── framework.h            ← Header chung
│   ├── Core.cpp               ← File mặc định wizard tạo (chưa dùng)
│   ├── include/
│   │   ├── ProcessInfo.h      ← struct mô tả 1 process
│   │   └── ProcessManager.h   ← class chính
│   └── src/
│       └── ProcessManager.cpp ← implementation
│
├── GUI/                       ← Project C++/CLI WinForms
│   ├── GUI.vcxproj
│   ├── GUI.vcxproj.filters
│   ├── MainForm.h             ← Form chính (class managed)
│   ├── MainForm.cpp           ← chỉ #include "MainForm.h"
│   ├── MainForm.resx          ← Resource (string, icon) của form
│   └── Program.cpp            ← Entry point main()
│
├── mockups/
│   └── 01_process_list.html   ← Mockup HTML cho ProcessListForm
│
└── docs/                      ← Folder này
    ├── 01_TONG_QUAN.md
    ├── 02_KIEN_THUC_NEN.md
    └── 03_GIAI_THICH_CODE.md  ← Bạn đang đọc
```

---

## B. `Core/include/ProcessInfo.h`

**Mục đích**: định nghĩa "khuôn dạng" 1 process. Khi enumerateProcesses() trả về 200 process, mỗi cái là 1 `ProcessInfo`.

```cpp
struct ProcessInfo {
    uint32_t    pid          = 0;       // ID số nguyên
    std::wstring name;                  // "chrome.exe"
    std::wstring path;                  // "C:\\Program Files\\..."
    bool        is64Bit      = false;   // true nếu x64
    uint64_t    workingSetKB = 0;       // RAM đang dùng (KB)
    bool        isAccessible = true;    // false nếu OS không cho mở
};
```

**Giải thích**:
- `struct` chứ không phải `class`: vì đây là POD (Plain Old Data) — chỉ chứa dữ liệu, không có method ảo. POD dễ truyền qua boundary native↔managed.
- `std::wstring` thay vì `std::string`: Windows API dùng UTF-16 (`wchar_t`), nên ta dùng wide string. Tên file `Café.exe` hoạt động đúng với wstring, sai với string.
- `uint32_t` thay vì `int`: PID không thể âm. Dùng unsigned cho rõ ràng.
- `uint64_t workingSetKB`: RAM của process có thể > 4GB → cần 64-bit. Đơn vị KB cho gọn (đỡ phải hiển thị byte).
- Init mặc định `= 0`, `= false`: tránh undefined behavior nếu quên gán.

---

## C. `Core/include/ProcessManager.h`

**Mục đích**: khai báo (declare) các method ta sẽ implement ở .cpp.

```cpp
class ProcessManager {
public:
    static bool enableDebugPrivilege();
    static std::vector<ProcessInfo> enumerateProcesses();
    static HANDLE openForReadWrite(uint32_t pid);
    static void closeHandle(HANDLE h);
    static bool isProcessAlive(HANDLE h);
};
```

**Tại sao `static`?**
- Không cần `new ProcessManager()` rồi gọi method. Gọi thẳng `ProcessManager::enumerateProcesses()`.
- Vì các method này không lưu state riêng — mỗi lần gọi là 1 transaction độc lập.
- Tương tự `Math.sqrt()` trong C#: không cần tạo instance Math.

**Vì sao tách `.h` và `.cpp`?**
- `.h` là "lời hứa" (declaration): "tôi có 5 method với chữ ký thế này".
- `.cpp` là "thực hiện" (definition): code thực sự làm gì.
- Khi GUI muốn dùng → chỉ cần `#include "ProcessManager.h"` (đọc lời hứa) là biết cách gọi. Code thực sự nằm trong `Core.lib` đã biên dịch sẵn.
- → Compile nhanh hơn (không phải rebuild code mỗi lần ai đó #include).

---

## D. `Core/src/ProcessManager.cpp` — đi sâu

### Header includes

```cpp
#include "pch.h"
#include "ProcessManager.h"

#include <TlHelp32.h>      // Toolhelp32 snapshot APIs
#include <Psapi.h>         // GetProcessMemoryInfo

#pragma comment(lib, "Psapi.lib")
```

**Giải thích**:
- `pch.h` PHẢI ở dòng đầu (precompiled header rule). Bỏ qua sẽ báo lỗi `C1010`.
- `TlHelp32.h`: header cho API liệt kê process.
- `Psapi.h`: header cho memory info.
- `#pragma comment(lib, "Psapi.lib")`: bảo linker tự link Psapi.lib. Tương đương với "Linker → Input → Additional Dependencies" trong project properties — nhưng tiện hơn vì nằm trong code.

### Method 1: `enableDebugPrivilege()`

**Mục đích**: bật quyền debug cho process tool. Cần để mở handle vào process system.

**Quy trình 3 bước**:
```cpp
// Bước 1: lấy access token của TOOL hiện tại
HANDLE hToken;
OpenProcessToken(GetCurrentProcess(),
                 TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                 &hToken);

// Bước 2: tra cứu LUID (giống "mã" của privilege)
LUID luid;
LookupPrivilegeValueW(nullptr, SE_DEBUG_NAME, &luid);

// Bước 3: gán privilege vào token với cờ ENABLED
TOKEN_PRIVILEGES tp{};
tp.PrivilegeCount = 1;
tp.Privileges[0].Luid = luid;
tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
AdjustTokenPrivileges(hToken, FALSE, &tp, 0, nullptr, nullptr);
```

**Pitfall quan trọng**:
```cpp
DWORD err = GetLastError();   // ← ĐỌC NGAY trước khi gọi API khác
CloseHandle(hToken);
return ok && err != ERROR_NOT_ALL_ASSIGNED;
```

`AdjustTokenPrivileges` thường return TRUE ngay cả khi privilege bị từ chối! Phải check `GetLastError()` → nếu là `ERROR_NOT_ALL_ASSIGNED` thì user không có quyền (chưa chạy as Administrator).

### Method 2: `enumerateProcesses()` — phần dài nhất

**Quy trình**:

```cpp
// 1. Tạo snapshot process table
HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

// 2. Khai báo entry, SET dwSize TRƯỚC (bắt buộc!)
PROCESSENTRY32W entry{};
entry.dwSize = sizeof(entry);

// 3. Lấy entry đầu tiên + lặp
if (!Process32FirstW(hSnap, &entry)) { ... }
do {
    // Xử lý entry...
} while (Process32NextW(hSnap, &entry));

CloseHandle(hSnap);
```

**Tại sao phải `entry.dwSize = sizeof(entry)`?**
- API này tồn tại từ Win95. Qua các version Windows, struct `PROCESSENTRY32W` có thể được mở rộng (thêm field).
- API check `dwSize` để biết phiên bản struct nào ta đang dùng → fill data phù hợp.
- Nếu không set, `dwSize = 0` → API từ chối, trả `FALSE`.

**Bên trong vòng lặp**: với mỗi process, ta mở handle nhẹ để hỏi thêm info.

```cpp
HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION,
                           FALSE,    // không kế thừa handle cho con
                           info.pid);
```

**Tại sao dùng `PROCESS_QUERY_LIMITED_INFORMATION` chứ không `PROCESS_QUERY_INFORMATION`?**
- Vista trở đi, có version "LIMITED" cấp quyền THẤP hơn.
- Quyền thấp → ít process từ chối → nhiều process hơn vào danh sách.
- Mục tiêu: chỉ hỏi info hiển thị (tên, RAM, bitness), không cần đọc memory.

Sau đó lấy 3 thông tin:

```cpp
// (a) Full path
wchar_t pathBuf[MAX_PATH];
DWORD pathLen = MAX_PATH;
QueryFullProcessImageNameW(hProc, 0, pathBuf, &pathLen);

// (b) Bitness
BOOL isWow64;
IsWow64Process(hProc, &isWow64);
info.is64Bit = !isWow64;

// (c) RAM usage
PROCESS_MEMORY_COUNTERS pmc{};
GetProcessMemoryInfo(hProc, &pmc, sizeof(pmc));
info.workingSetKB = pmc.WorkingSetSize / 1024;
```

**Hiểu `IsWow64Process`**:
- `Wow64` = Windows on Windows 64. Là lớp giả lập cho phép process 32-bit chạy trên Windows 64-bit.
- `isWow64 = TRUE` → process là 32-bit (chạy qua WoW64)
- `isWow64 = FALSE` → process là 64-bit thật
- Vậy `is64Bit = !isWow64`.

### Method 3: `openForReadWrite()`

```cpp
HANDLE ProcessManager::openForReadWrite(uint32_t pid) {
    DWORD desired = PROCESS_VM_READ
                  | PROCESS_VM_WRITE
                  | PROCESS_VM_OPERATION
                  | PROCESS_QUERY_INFORMATION;
    return OpenProcess(desired, FALSE, pid);
}
```

**Bit OR (|)** các flag = "tôi muốn TẤT CẢ những quyền này". OS kiểm tra từng cái.

Đây là quyền CAO — sẽ bị từ chối với process bảo vệ. Đó là lý do hàm này chỉ gọi khi user chủ động chọn process (chứ không gọi cho cả 200 process khi enumerate).

### Method 4: `closeHandle()` (wrapper an toàn)

```cpp
void ProcessManager::closeHandle(HANDLE h) {
    if (h && h != INVALID_HANDLE_VALUE) {
        CloseHandle(h);
    }
}
```

Win32 `CloseHandle(nullptr)` thực ra crash hoặc set last-error. Wrapper này check trước cho an toàn.

### Method 5: `isProcessAlive()`

```cpp
DWORD exitCode = 0;
GetExitCodeProcess(h, &exitCode);
return exitCode == STILL_ACTIVE;
```

`GetExitCodeProcess` trả về `STILL_ACTIVE (= 259)` nếu process còn chạy. Edge case: process exit với code đúng 259 → vẫn nhận là alive. Hiếm, chấp nhận được.

---

## E. `GUI/MainForm.h` — Form chính

Wizard tạo, ta chưa sửa.

```cpp
namespace GUI {
    public ref class MainForm : public System::Windows::Forms::Form {
    public:
        MainForm(void) {
            InitializeComponent();
        }
    protected:
        ~MainForm() { /* cleanup */ }
    private:
        System::ComponentModel::IContainer^ components;

        #pragma region Windows Form Designer generated code
        void InitializeComponent(void) {
            this->SuspendLayout();
            // ... designer fills here
            this->ResumeLayout(false);
        }
        #pragma endregion
    };
}
```

**Giải thích**:
- `namespace GUI`: tên namespace = tên project. Để tránh xung đột tên với .NET (vd: System::Windows::Forms::Form).
- `public ref class`: managed class. Nếu chỉ viết `public class` → là native class (sẽ lỗi vì kế thừa từ managed Form).
- `: public System::Windows::Forms::Form`: kế thừa Form chuẩn của .NET. Tự động có title bar, minimize/maximize, drag để di chuyển,...
- `InitializeComponent()`: method đặc biệt Designer dùng. Mỗi khi bạn kéo thả button vào form, Designer thêm dòng code vào đây.
- `#pragma region ... #pragma endregion`: marker cho Designer. **TUYỆT ĐỐI không sửa code giữa 2 marker này bằng tay** — Designer sẽ ghi đè. Muốn custom thì viết OUTSIDE region.

---

## F. `GUI/Program.cpp` — Entry point

```cpp
#include "MainForm.h"
using namespace System;
using namespace System::Windows::Forms;

[STAThreadAttribute]
int main(array<System::String^>^ args) {
    Application::EnableVisualStyles();
    Application::SetCompatibleTextRenderingDefault(false);
    Application::Run(gcnew GUI::MainForm());
    return 0;
}
```

**Từng dòng**:

| Dòng | Ý nghĩa |
|------|---------|
| `[STAThreadAttribute]` | Attribute đánh dấu main thread là **Single-Threaded Apartment**. Bắt buộc cho WinForms (vì WinForms dùng COM). |
| `int main(array<System::String^>^ args)` | Entry point dạng managed. `array<String^>^` = mảng managed của String. Native main là `int main(int argc, char* argv[])` — khác. |
| `EnableVisualStyles()` | Bật visual style Win XP+ (button có gradient nhẹ, scroll bar đẹp). Bỏ → button trông như Win95. |
| `SetCompatibleTextRenderingDefault(false)` | Dùng GDI+ rendering (đẹp hơn GDI cũ). |
| `Application::Run(gcnew GUI::MainForm())` | Tạo form mới, bật message loop của Windows, hiển thị form. Loop block đây cho tới khi form đóng. |
| `return 0` | Thoát process với exit code 0 = thành công. |

**Application::Run là gì sâu hơn?**
Win32 GUI dựa trên **message loop**:
```
Win32 native (sau khi compile từ C++/CLI):
while (GetMessage(&msg, ...)) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);    // gọi WindowProc của form
}
```
`Application::Run` ẩn loop này dưới layer .NET. Khi user di chuột, gõ phím, click button → Windows gửi message → loop bắt → dispatch tới form → event handler trong form code chạy.

---

## G. Build flow — cái gì xảy ra khi bấm `Ctrl+Shift+B`

```
1. MSBuild đọc CheatVN.sln
       → tìm Core và GUI vcxproj
       → biết GUI depends on Core → build Core trước

2. Build Core:
       → Compile pch.cpp (tạo precompiled header)
       → Compile Core.cpp, ProcessManager.cpp (dùng pch để nhanh)
       → Linker gom lại → Core.lib (~50KB)
       → Output: CheatVN/x64/Debug/Core.lib

3. Build GUI:
       → Compile MainForm.cpp, Program.cpp (với /clr flag)
       → Compile ra .NET MSIL + native code lẫn lộn
       → Linker:
            • Link với .NET assemblies (System.Windows.Forms.dll,...)
            • Link với Core.lib (gọi native function)
            • Tạo executable
       → Output: CheatVN/x64/Debug/GUI.exe (~100KB)

4. Run (F5):
       → Windows load GUI.exe
       → CLR khởi tạo .NET runtime
       → main() chạy → MainForm hiện
```

---

## Đọc tiếp

Sau khi đọc xong 3 file này:
- Mở `mockups/01_process_list.html` trong browser → xem hình dung UI
- Chạy `F5` xem cửa sổ rỗng hiện ra
- Báo mình **đang ở đâu** trong tài liệu để bắt đầu Phase 1d: viết Bridge + design ProcessListForm

Nếu có khái niệm nào chưa rõ → hỏi mình, mình giải thích sâu hơn cho riêng cái đó.

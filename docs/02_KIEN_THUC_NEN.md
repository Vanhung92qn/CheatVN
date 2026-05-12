# 02 — Kiến thức nền tảng

> Đọc cái này để hiểu **vocabulary** trước khi xem code. Có 3 mảng: VS / Win32 / C++/CLI.

---

## A. Visual Studio: Solution, Project, File

VS có **3 cấp container**:

```
Solution (.sln)            ← cấp cao nhất, chứa nhiều project
   └── Project (.vcxproj)  ← 1 đơn vị build (1 exe hoặc 1 lib)
          └── File (.cpp/.h)  ← code thật
```

### Solution (`.sln`)

- Là **file text** plain (mở bằng Notepad đọc được).
- Chứa danh sách project và config build (Debug/Release, x64/x86).
- KHÔNG chứa code, chỉ là "danh bạ".

CheatVN.sln liệt kê 2 project: Core + GUI.

### Project (`.vcxproj`)

- Là **file XML**, mô tả cách build 1 unit.
- Khai báo: compile gì, link gì, define gì, target framework gì.
- Mỗi project tạo ra 1 file output:
  - **Core.vcxproj** → `Core.lib` (static library — file chứa code biên dịch sẵn)
  - **GUI.vcxproj** → `GUI.exe` (chương trình chạy được)

### Tại sao Core là `.lib` chứ không phải `.exe`?

Vì Core không chạy độc lập — nó là **thư viện**, được "nhúng" vào GUI khi build. Cuối cùng chỉ có 1 file `GUI.exe` chứa toàn bộ code của cả 2.

```
Core.cpp ──┐
           ├──► Core.lib ──┐
ProcessMgr.cpp ┘            ├──► GUI.exe (1 file duy nhất chạy)
                            │
MainForm.cpp ────────────────┤
Program.cpp ─────────────────┘
```

### Configuration: Debug vs Release, x86 vs x64

| Config | Đặc điểm |
|--------|----------|
| **Debug** | Code chậm, có symbol để debug, có check runtime (vd: detect buffer overflow) |
| **Release** | Compiler tối ưu mạnh, code nhanh, không có debug info |
| **x86** | Build 32-bit. Tool 32-bit chỉ scan được process 32-bit. |
| **x64** | Build 64-bit. Tool 64-bit chỉ scan được process 64-bit. |

→ Mình chọn **x64** vì 99% software hiện đại đều 64-bit.

---

## B. Win32 — Cách Windows quản lý process

### Process là gì?

Một **process** = 1 chương trình đang chạy. Mỗi process có:
- **PID** (Process ID): số nguyên unique (vd: 1234)
- **Virtual address space**: vùng nhớ ảo riêng (process A không thấy memory của process B)
- **Threads**: 1 hoặc nhiều luồng thực thi
- **Handles**: tham chiếu tới các tài nguyên đang mở (file, socket, registry key,...)
- **Access Token**: chứng minh thư của process — chứa user ID và privileges

### Virtual Memory (cốt lõi)

```
Process A                Process B
┌─────────────┐         ┌─────────────┐
│ 0x00000000  │         │ 0x00000000  │
│   ...       │  KHÔNG  │   ...       │  ← cùng địa chỉ ảo
│ 0x1000_0000 │ THẤY    │ 0x1000_0000 │     nhưng map vào
│   value=50  │  ───X──►│   value=99  │     RAM vật lý khác
│   ...       │  ◄──X───│   ...       │
│ 0xFFFFFFFF  │         │ 0xFFFFFFFF  │
└─────────────┘         └─────────────┘
```

→ Mặc định, process A không thể đọc memory của process B. **OS bảo vệ tuyệt đối**.

→ Để CheatVN đọc/ghi memory của process khác, ta dùng **Win32 cross-process API**: `ReadProcessMemory` / `WriteProcessMemory`. Đây là "đặc quyền" mà OS cho phép vì có nhu cầu hợp pháp (debugger, antivirus, profiler).

### Handle là gì?

**Handle** là 1 con số (vd: `0x000001A4`) đại diện cho "quyền truy cập đến 1 tài nguyên". Tương tự như "vé gửi xe" — bạn không trực tiếp cầm xe, chỉ cầm vé.

```cpp
HANDLE h = OpenProcess(PROCESS_VM_READ, FALSE, 1234);
//        ↑ Win32 trả về handle nếu OS cho phép
//        ↑ Từ giờ dùng h để đọc memory của PID 1234
```

→ Khi xong việc, PHẢI `CloseHandle(h)` để OS giải phóng. Quên = leak.

### Access Rights — quyền của handle

Khi mở handle, bạn phải khai báo **muốn làm gì với nó**. OS cấp quyền hoặc từ chối.

| Flag | Cho phép | Tương tự |
|------|----------|----------|
| `PROCESS_QUERY_LIMITED_INFORMATION` | Hỏi info cơ bản (tên, path, bitness) | Mượn xem lý lịch |
| `PROCESS_QUERY_INFORMATION` | Hỏi nhiều info hơn | Xem hồ sơ chi tiết |
| `PROCESS_VM_READ` | Đọc memory | Đọc nhật ký |
| `PROCESS_VM_WRITE` | Ghi memory | Sửa nhật ký |
| `PROCESS_VM_OPERATION` | Đổi page protection | Bẻ khóa tủ |

→ Quyền càng cao càng dễ bị OS từ chối (vd: với antivirus process). Nguyên tắc: **xin quyền tối thiểu cần thiết**.

### Privilege — đặc quyền của process tool

OS chia user thành nhiều cấp:
- User thường: chỉ truy cập process của mình
- Administrator: truy cập process của user khác
- **Với SeDebugPrivilege**: truy cập cả process system (services, antivirus)

`SeDebugPrivilege` không tự bật — phải code gọi `AdjustTokenPrivileges()` để bật. Và chỉ bật được nếu user chạy app **as Administrator**.

→ Code `enableDebugPrivilege()` trong Core làm việc này.

---

## C. C++/CLI — Cây cầu nối Native ↔ Managed

### Hai thế giới song song

| | Native C++ | Managed C++/CLI |
|-|------------|-----------------|
| Compile ra | Mã máy (x86/x64 instructions) | Mã CIL (Common Intermediate Language) |
| Runtime | Chạy thẳng CPU | Chạy trên .NET CLR (Common Language Runtime) |
| Bộ nhớ | new/delete thủ công | Garbage Collector tự dọn |
| Đặc tính | Nhanh, ít overhead | An toàn, dễ viết UI |
| Tốc độ | 100% (baseline) | ~70-90% so với native |

→ C++/CLI là phiên bản C++ mở rộng để **viết được CẢ HAI** trong cùng file.

### Syntax khác biệt

Native C++:
```cpp
std::string name;              // Bộ nhớ stack
ProcessInfo* p = new ProcessInfo();  // Bộ nhớ heap, tự delete
delete p;
```

Managed C++/CLI:
```cpp
String^ name;                  // ^ = "handle to managed object"
ProcessInfo^ p = gcnew ProcessInfo();  // gcnew = managed new, GC tự dọn
// KHÔNG delete — GC làm tự động
```

| Native | Managed | Ý nghĩa |
|--------|---------|---------|
| `Type*` | `Type^` | Pointer/handle |
| `new` | `gcnew` | Cấp phát object |
| `class` | `ref class` | Class trên managed heap |
| `delete` | `(không cần)` | GC tự xóa |

### Mixing trong cùng file

Bạn có thể có cả 2 loại trong 1 file:

```cpp
// File: ProcessManagerBridge.h (sẽ viết ở Phase 1d)

#include "ProcessManager.h"  // native header

namespace GUI {

    // Managed class — GUI gọi được
    public ref class ProcessManagerBridge {
    public:
        static List<String^>^ ListProcesses() {
            // Bên trong gọi native code:
            auto native = cheatvn::ProcessManager::enumerateProcesses();

            // Convert từ native vector sang managed List:
            auto result = gcnew List<String^>();
            for (auto& info : native) {
                String^ name = gcnew String(info.name.c_str());
                result->Add(name);
            }
            return result;
        }
    };
}
```

→ Đây là **"bridge"**: nhìn từ GUI thì nó là class managed bình thường. Nhưng bên trong gọi native code. Marshal data 1 lần ở boundary.

### Pinning — khi data managed muốn truyền vào native

Khi GUI có `String^` cần truyền vào hàm native nhận `const wchar_t*`:

```cpp
String^ managedStr = "hello";

// Pin (ghim) managed string để GC không di chuyển nó
pin_ptr<const wchar_t> nativePtr = PtrToStringChars(managedStr);

// Giờ truyền nativePtr vào hàm native an toàn
nativeFunction(nativePtr);
```

→ Đây là kỹ thuật advanced, mình sẽ giải thích khi dùng đến.

---

## Đọc tiếp

- 📘 **[03 — Giải thích code đã viết](03_GIAI_THICH_CODE.md)**

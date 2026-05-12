#pragma once

// =====================================================================
//  ProcessManagerBridge.h
//
//  Đây là "cây cầu" (Bridge) giữa 2 thế giới trong cùng project GUI:
//
//      [Native C++ thuần]          [Managed C++/CLI]
//      cheatvn::ProcessInfo  ──►   ManagedProcessInfo (ref class)
//      std::vector<...>      ──►   List<...>^
//      std::wstring          ──►   String^
//      HANDLE                ──►   IntPtr
//
//  Tại sao cần bridge?
//  ────────────────────
//  - DataGridView của Windows Forms CHỈ binding được vào managed object
//    (ref class) — nó không hiểu std::vector hay native struct.
//  - String^ là wrapper managed của UTF-16; std::wstring là native.
//  - Native HANDLE là void* — managed dùng IntPtr (tương đương kích thước
//    nhưng managed-aware).
//
//  Quy tắc: code form chỉ gọi đến đây, KHÔNG include native header trực
//  tiếp. Như vậy code form sạch managed, dễ thay đổi bridge sau này.
// =====================================================================

namespace GUI {

    // ─────────────────────────────────────────────────────────────────
    //  ManagedProcessInfo — bản managed của ProcessInfo native.
    //
    //  Mỗi field là một "property" managed (.NET style). Property khác
    //  field thường ở chỗ: có thể có getter/setter, và quan trọng nhất —
    //  DataGridView dùng reflection để binding theo TÊN PROPERTY.
    //
    //  Ví dụ: cột grid có DataPropertyName = "Name" → DGV tự gọi
    //         .Name (getter) trên mỗi object để hiển thị.
    // ─────────────────────────────────────────────────────────────────
    public ref class ManagedProcessInfo {
    public:
        // ─── Properties cơ bản ─────────────────────────────────────
        // Cú pháp `property Type Name;` = auto-property (.NET style),
        // tự sinh ra getter+setter ngầm. Tương đương với:
        //     private: Type _name;
        //     public: Type Name { Type get(){return _name;} ...

        property System::UInt32 Pid;            // Process ID
        property System::String^ Name;          // "chrome.exe"
        property System::String^ Path;          // "C:\Program Files\..."
        property bool Is64Bit;                  // true = x64
        property System::UInt64 WorkingSetKB;   // RAM (KB)
        property bool IsAccessible;             // false = process system bảo vệ

        // ─── Properties tính toán (read-only) ──────────────────────
        // Dạng property có getter explicit để format dữ liệu cho DGV
        // hiển thị đẹp. Ví dụ "1.2 GB" thay vì 1258291.

        // Trả "x64" hoặc "x86" — DGV binding cột Bitness vào đây.
        property System::String^ Bitness {
            System::String^ get();
        }

        // Trả "12 KB" / "1.2 MB" / "3.45 GB" — format theo độ lớn.
        property System::String^ RamDisplay {
            System::String^ get();
        }

        // Trả "Sẵn sàng" hoặc "Cần quyền" — DGV binding cột Trạng thái.
        property System::String^ Status {
            System::String^ get();
        }
    };

    // ─────────────────────────────────────────────────────────────────
    //  ProcessManagerBridge — lớp tĩnh, không có instance.
    //
    //  Cú pháp `ref class Foo abstract sealed`:
    //    - abstract → không tạo instance được
    //    - sealed   → không kế thừa được
    //    Kết hợp ⇒ tương đương `static class` trong C#.
    //    Tất cả method bên trong PHẢI là static.
    //
    //  Đây là entry point duy nhất để code form gọi vào Core. Mỗi method
    //  sẽ wrap 1 hàm tương ứng trong cheatvn::ProcessManager.
    // ─────────────────────────────────────────────────────────────────
    public ref class ProcessManagerBridge abstract sealed {
    public:
        // Bật SeDebugPrivilege cho tool. Cần Admin. Gọi 1 lần khi khởi động.
        // Trả false nếu user không có quyền (chạy as user thường).
        static bool EnableDebugPrivilege();

        // Liệt kê toàn bộ process. Mỗi lần gọi tốn ~30-100ms tùy số process.
        // CHẶN thread gọi — UI sẽ đứng. Sau này ta gọi trong BackgroundWorker
        // để không block UI thread.
        static System::Collections::Generic::List<ManagedProcessInfo^>^
            EnumerateProcesses();

        // Mở handle vào process với quyền đầy đủ (read+write+operation).
        // IntPtr là kiểu managed tương đương void* trong native — chứa được
        // địa chỉ pointer hoặc HANDLE.
        // Trả IntPtr::Zero nếu fail.
        static System::IntPtr OpenForReadWrite(System::UInt32 pid);

        // Đóng handle. Wrapper an toàn — gọi với IntPtr::Zero không crash.
        static void CloseHandle(System::IntPtr handle);

        // Check process còn chạy không (qua exit code).
        static bool IsProcessAlive(System::IntPtr handle);
    };

} // namespace GUI

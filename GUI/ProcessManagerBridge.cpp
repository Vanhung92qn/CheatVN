// =====================================================================
//  ProcessManagerBridge.cpp
//
//  Implementation của các method bridge — chỗ thực sự DIỄN RA việc marshal
//  giữa native và managed.
//
//  File này được compile với /clr (do project GUI có CLRSupport=true),
//  cho phép trộn lẫn 2 thế giới:
//    - std::vector, std::wstring  → native (chạy thẳng CPU)
//    - String^, List<T>^, gcnew    → managed (chạy trên .NET CLR)
// =====================================================================

#include "ProcessManagerBridge.h"

// ⚠ THỨ TỰ INCLUDE QUAN TRỌNG:
//   Native header phải include TRƯỚC khi using namespace .NET. Nếu đảo
//   ngược, compiler có thể nhầm tên (vd: std::string vs System::String).
#include "ProcessManager.h"   // Native — gọi sang Core

using namespace System;
using namespace System::Collections::Generic;

namespace GUI {

    // =================================================================
    //  ManagedProcessInfo — implementation các computed properties
    // =================================================================

    String^ ManagedProcessInfo::Bitness::get() {
        // Toán tử ternary của C++/CLI giống C# — String^ literal tự
        // được tạo thành managed String.
        return Is64Bit ? "x64" : "x86";
    }

    String^ ManagedProcessInfo::RamDisplay::get() {
        // Format RAM theo độ lớn cho dễ đọc.
        // ".ToString(\"F1\")" = format số thập phân 1 chữ số sau dấu phẩy.
        // VD: 1572864 KB → 1.5 GB
        if (WorkingSetKB < 1024) {
            // < 1 MB → hiển thị KB
            return WorkingSetKB.ToString() + " KB";
        }
        if (WorkingSetKB < 1024ULL * 1024ULL) {
            // < 1 GB → hiển thị MB
            double mb = (double)WorkingSetKB / 1024.0;
            return mb.ToString("F1") + " MB";
        }
        // ≥ 1 GB → hiển thị GB
        double gb = (double)WorkingSetKB / 1024.0 / 1024.0;
        return gb.ToString("F2") + " GB";
    }

    String^ ManagedProcessInfo::Status::get() {
        return IsAccessible ? L"Sẵn sàng" : L"Cần quyền";
    }

    // =================================================================
    //  ProcessManagerBridge — implementation static methods
    // =================================================================

    bool ProcessManagerBridge::EnableDebugPrivilege() {
        // Gọi thẳng native — bool→bool trivial, không cần marshal.
        return cheatvn::ProcessManager::enableDebugPrivilege();
    }

    List<ManagedProcessInfo^>^ ProcessManagerBridge::EnumerateProcesses() {
        // ─── Bước 1: gọi native, lấy std::vector<ProcessInfo> ───────
        // Native code chạy không có overhead managed → siêu nhanh.
        // Trả về 1 lần dưới dạng vector (mỗi phần tử là native struct).
        std::vector<cheatvn::ProcessInfo> nativeList =
            cheatvn::ProcessManager::enumerateProcesses();

        // ─── Bước 2: tạo List managed, reserve capacity ─────────────
        // gcnew = managed new. Sẽ được Garbage Collector dọn tự động.
        // Capacity trước = tối ưu — tránh resize động khi Add nhiều.
        auto result = gcnew List<ManagedProcessInfo^>();
        result->Capacity = (int)nativeList.size();

        // ─── Bước 3: duyệt vector, marshal từng phần tử sang managed
        // Đây là chỗ DUY NHẤT có overhead marshal — chấp nhận được vì
        // chỉ 200-300 process (làm 1 lần khi enumerate).
        //
        // Lưu ý: KHÔNG marshal trong vòng lặp nóng (vd: scan 4GB memory)
        // vì mỗi gcnew + String^ tạo gánh nặng GC.
        for (const auto& info : nativeList) {
            // gcnew tạo object trên managed heap (GC quản lý)
            auto m = gcnew ManagedProcessInfo();

            // Copy field cơ bản (POD types)
            m->Pid = info.pid;
            m->Is64Bit = info.is64Bit;
            m->WorkingSetKB = info.workingSetKB;
            m->IsAccessible = info.isAccessible;

            // Marshal std::wstring → String^.
            // gcnew String(const wchar_t*) là constructor có sẵn của
            // System::String — copy data từ native sang managed heap.
            m->Name = gcnew String(info.name.c_str());
            m->Path = gcnew String(info.path.c_str());

            // List<T>::Add tự increment size, không cần manage thủ công.
            result->Add(m);
        }

        // Native nativeList ra khỏi scope → destructor std::vector tự dọn.
        // result^ tồn tại trên managed heap → trả về GUI safely.
        return result;
    }

    IntPtr ProcessManagerBridge::OpenForReadWrite(UInt32 pid) {
        // ─ Bước 1: gọi native, được HANDLE (= void* trong native) ─
        HANDLE h = cheatvn::ProcessManager::openForReadWrite(pid);

        // ─ Bước 2: gói void* vào IntPtr (kiểu managed) ─
        // IntPtr là wrapper managed quanh native pointer. Trên 64-bit
        // build, IntPtr = 8 bytes (đủ chứa pointer 64-bit). Trên 32-bit
        // build, IntPtr = 4 bytes. Tự match.
        return IntPtr(h);
    }

    void ProcessManagerBridge::CloseHandle(IntPtr handle) {
        // ToPointer() lấy ra void* gốc từ IntPtr.
        // Cast về HANDLE (= void*) rồi gọi native wrapper.
        cheatvn::ProcessManager::closeHandle(handle.ToPointer());
    }

    bool ProcessManagerBridge::IsProcessAlive(IntPtr handle) {
        return cheatvn::ProcessManager::isProcessAlive(handle.ToPointer());
    }

} // namespace GUI

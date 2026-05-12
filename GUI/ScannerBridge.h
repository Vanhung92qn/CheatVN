#pragma once

// =====================================================================
//  ScannerBridge.h — cầu nối managed↔native cho Scanner.
//
//  Pattern marshal:
//    Native cheatvn::ScanValue (union)  ⇄  Int64 "raw bits"
//
//  Tại sao dùng Int64 raw thay vì gói thành ManagedScanValue^?
//   - Hiệu năng: scan trả về 4 triệu địa chỉ → 4 triệu gcnew = chậm
//   - Đơn giản: Int64 fit được mọi primitive 8-byte trở xuống
//     * Int8/16/32: zero/sign-extend
//     * Int64: lưu thẳng
//     * Float: lưu IEEE 754 bit pattern qua reinterpret
//     * Double: lưu IEEE 754 8-byte
//   - Binding DGV: ManagedScanResult.CurrentDisplay decode raw → string
// =====================================================================

namespace GUI {

    // ─────────────────────────────────────────────────────────────────
    //  Managed mirrors của native enum
    //  (Giá trị số PHẢI khớp với cheatvn::ValueType / ScanOperator
    //   để cast đơn giản)
    // ─────────────────────────────────────────────────────────────────
    public enum class ManagedValueType {
        Int8      = 0,
        Int16     = 1,
        Int32     = 2,
        Int64     = 3,
        Float     = 4,
        Double    = 5,
        String    = 6,
        ByteArray = 7
    };

    public enum class ManagedScanOperator {
        // So với target value
        Exact          = 0,
        NotEqual       = 1,
        Greater        = 2,
        GreaterOrEqual = 3,
        Less           = 4,
        LessOrEqual    = 5,
        Between        = 6,
        // So với previous value (chỉ cho next scan)
        Changed        = 7,
        Unchanged      = 8,
        Increased      = 9,
        Decreased      = 10,
        IncreasedBy    = 11,
        DecreasedBy    = 12
    };

    // ─────────────────────────────────────────────────────────────────
    //  ManagedScanResult — 1 row trong kết quả scan (sẽ binding lên DGV)
    //
    //  Mỗi field là property cho DGV reflection. Address/CurrentRaw/...
    //  là dữ liệu thô; *Display là chuỗi format để hiển thị.
    // ─────────────────────────────────────────────────────────────────
    public ref class ManagedScanResult {
    public:
        property System::UInt64        Address;       // địa chỉ memory
        property ManagedValueType      Type;          // kiểu dữ liệu
        property System::Int64         CurrentRaw;    // giá trị hiện tại (raw bits)
        property System::Int64         PreviousRaw;   // giá trị scan trước (raw bits)
        property bool                  HasPrevious;   // false sau firstScan

        // ─── Freeze state ─────────────────────────────────────
        // IsFrozen: true → timer ở MainForm sẽ ghi FrozenRaw vào address
        //           liên tục mỗi 50ms (giữ giá trị "đóng băng")
        // FrozenRaw: giá trị muốn lock. Khi user check checkbox, ta copy
        //           CurrentRaw vào đây. User có thể edit value để đổi
        //           giá trị frozen.
        property bool                  IsFrozen;
        property System::Int64         FrozenRaw;

        // Display properties — DGV binding theo tên này
        property System::String^ AddressDisplay { System::String^ get(); }
        property System::String^ CurrentDisplay { System::String^ get(); }
        property System::String^ PreviousDisplay { System::String^ get(); }
    };

    // ─────────────────────────────────────────────────────────────────
    //  ManagedMemoryRegion — wrap native MemoryRegion cho UI heatmap
    // ─────────────────────────────────────────────────────────────────
    public ref class ManagedMemoryRegion {
    public:
        property System::UInt64 BaseAddress;
        property System::UInt64 Size;
        property System::UInt32 Protection;
        property System::UInt32 RegionState;
        property System::UInt32 RegionType;
        property bool IsReadable;
        property bool IsWritable;
        property bool IsImage;     // .exe/.dll loaded
        property bool IsPrivate;   // heap/stack/private alloc
        property bool IsMapped;    // file mapping

        // Display props
        property System::String^ AddressDisplay {
            System::String^ get() { return "0x" + BaseAddress.ToString("X16"); }
        }
        property System::String^ SizeDisplay {
            System::String^ get() {
                if (Size < 1024) return Size + " B";
                if (Size < 1024 * 1024) return (Size / 1024.0).ToString("F1") + " KB";
                if (Size < 1024ULL * 1024 * 1024) return (Size / 1024.0 / 1024.0).ToString("F1") + " MB";
                return (Size / 1024.0 / 1024.0 / 1024.0).ToString("F2") + " GB";
            }
        }
        property System::String^ TypeDisplay {
            System::String^ get() {
                if (IsImage) return "Image (exe/dll)";
                if (IsPrivate) return "Private (heap/stack)";
                if (IsMapped) return "Mapped (file)";
                return "Other";
            }
        }
        property System::String^ ProtectionDisplay {
            System::String^ get() {
                System::String^ s = "";
                if (IsReadable) s += "R";
                if (IsWritable) s += "W";
                if (s == "") s = "—";
                return s;
            }
        }
    };

    // ─────────────────────────────────────────────────────────────────
    //  ScannerBridge — static class, entry point cho UI
    // ─────────────────────────────────────────────────────────────────
    public ref class ScannerBridge abstract sealed {
    public:
        // ─── Encode value → Int64 raw ───
        // UI gọi để chuyển input của user (int/float/...) sang format raw
        // truyền vào FirstScan / NextScan.
        static System::Int64 EncodeInt8(System::SByte v);
        static System::Int64 EncodeInt16(System::Int16 v);
        static System::Int64 EncodeInt32(System::Int32 v);
        static System::Int64 EncodeInt64(System::Int64 v);
        static System::Int64 EncodeFloat(float v);
        static System::Int64 EncodeDouble(double v);

        // ─── Parse string → Int64 raw (theo type) ───
        // Wrapper tiện: UI nhận input string từ TextBox.
        // Trả về (success, raw). success=false nếu parse fail.
        static bool TryParseValue(
            System::String^ text,
            ManagedValueType type,
            [System::Runtime::InteropServices::Out] System::Int64% outRaw);

        // ─── First scan ───
        // hProcess: handle có PROCESS_VM_READ (lấy từ ProcessManagerBridge::OpenForReadWrite)
        // type: kiểu giá trị
        // targetRaw: giá trị target đã encode
        // op: phép so sánh (chỉ Exact..LessOrEqual hợp lệ cho first scan)
        static System::Collections::Generic::List<ManagedScanResult^>^ FirstScan(
            System::IntPtr hProcess,
            ManagedValueType type,
            System::Int64 targetRaw,
            ManagedScanOperator op);

        // ─── Next scan ───
        // previous: list từ FirstScan/NextScan trước đó.
        // Caller có thể truyền chính list cũ; bridge sẽ duyệt và trả list mới.
        static System::Collections::Generic::List<ManagedScanResult^>^ NextScan(
            System::IntPtr hProcess,
            System::Collections::Generic::List<ManagedScanResult^>^ previous,
            ManagedValueType type,
            System::Int64 targetRaw,
            ManagedScanOperator op);

        // ─── Refresh current values cho list kết quả ───
        // Gọi từ Timer mỗi ~500ms để cập nhật DGV "live".
        // Modify in-place: cập nhật CurrentRaw của từng result.
        static void RefreshValues(
            System::IntPtr hProcess,
            System::Collections::Generic::List<ManagedScanResult^>^ results);

        // ─── Ghi giá trị mới vào memory (cho Edit value / Freeze) ───
        static bool WriteValue(
            System::IntPtr hProcess,
            System::UInt64 address,
            ManagedValueType type,
            System::Int64 valueRaw);

        // ─── Đọc N bytes raw tại 1 địa chỉ (cho Hex Viewer) ──────
        // Trả về byte array có length = số bytes đọc được (có thể < count
        // nếu chạm guard page hoặc region boundary).
        static cli::array<System::Byte>^ ReadBytes(
            System::IntPtr hProcess,
            System::UInt64 address,
            int count);

        // ─── First scan với cấu hình tùy chỉnh (cho Benchmark) ───
        // Cho phép control chính xác numThreads + useSimd để so sánh hiệu năng.
        // Trả về số kết quả tìm được (không cần list — benchmark chỉ đo time).
        static int FirstScanBenchmark(
            System::IntPtr hProcess,
            ManagedValueType type,
            System::Int64 targetRaw,
            ManagedScanOperator op,
            unsigned int numThreads,
            bool useSimd);

        // ─── Enumerate all readable memory regions của process ───
        // Dùng cho Memory Heatmap. Trả về List sorted theo BaseAddress.
        static System::Collections::Generic::List<ManagedMemoryRegion^>^
            EnumerateRegions(System::IntPtr hProcess);
    };

} // namespace GUI

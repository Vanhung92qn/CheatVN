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
    };

} // namespace GUI

#pragma once

#include "MemoryRegion.h"
#include "ScanTypes.h"
#include <vector>
#include <Windows.h>

namespace cheatvn {

// =====================================================================
//  MemoryScanner — class chính cho việc quét bộ nhớ.
//
//  Lifecycle 1 phiên scan:
//    1. enumerateRegions(hProc) → list các vùng nhớ đọc được
//    2. firstScan(hProc, target, op)
//          → đọc từng region, scan tìm value, trả về 1 list địa chỉ
//    3. (optional) nextScan(hProc, prevResults, newTarget, newOp)
//          → đọc lại các địa chỉ trong prevResults, lọc theo điều kiện mới
//          → repeat đến khi danh sách đủ nhỏ
//    4. readValue(hProc, addr, type) → đọc 1 giá trị tại 1 địa chỉ
//          (cho việc refresh giá trị hiển thị trên DGV)
//
//  Phase 2 baseline: single-threaded, scan aligned theo size of value.
//  Phase 2i-j: thêm multi-thread + SIMD.
// =====================================================================
class MemoryScanner {
public:
    // ─── Tùy chọn scan ──────────────────────────────────────────
    struct ScanOptions {
        // Scan aligned theo sizeof(T)? Nhanh ~4x cho int32. Bật mặc định.
        // Tắt nếu nghi value lưu unaligned (hiếm gặp).
        bool aligned = true;

        // Bỏ qua các region thuộc image (exe, dll) — chứa code, không phải data?
        // Bật → loại trừ code → nhanh hơn, ít noise. Nhưng cũng bỏ qua
        // static variable nằm trong .data section của exe. Mặc định: false
        // (scan tất cả) để không bỏ sót.
        bool skipImageRegions = false;

        // Bỏ qua region read-only? Memory read-only thường không bị thay đổi
        // → không cần scan. Default: false.
        bool skipReadOnly = false;

        // Bỏ qua region quá lớn (vd: file-mapping mấy GB)? Tránh OOM.
        // 0 = không giới hạn. Mặc định 256 MB.
        uint64_t maxRegionSize = 256ULL * 1024 * 1024;

        // Số thread dùng cho scan song song.
        // 0 = auto-detect (= std::thread::hardware_concurrency()).
        // Giảm xuống 1 nếu muốn debug đơn luồng.
        // Tăng lên hơn số core không có lợi (CPU bound, không IO bound).
        unsigned int numThreads = 0;
    };

    // ─── Liệt kê các vùng nhớ readable của process ───────────────
    // Dùng VirtualQueryEx duyệt không gian địa chỉ ảo.
    // Trả về vector các MEM_COMMIT region có thể đọc được.
    static std::vector<MemoryRegion> enumerateRegions(HANDLE hProcess);

    // ─── First scan: quét toàn bộ memory tìm value ───────────────
    // Đọc từng region với ReadProcessMemory, scan aligned, gom kết quả.
    // hProcess: handle có PROCESS_VM_READ.
    // target: giá trị muốn tìm (.type quy định kiểu).
    // op: phép so sánh (Exact/NotEqual/Greater/Less/GE/LE).
    // Note: First scan KHÔNG support Changed/Unchanged/Increased/...
    //       (vì chưa có previous data để so).
    static std::vector<ScanResult> firstScan(
        HANDLE hProcess,
        const ScanValue& target,
        ScanOperator op = ScanOperator::Exact,
        const ScanOptions& opts = ScanOptions()
    );

    // ─── Next scan: lọc kết quả lần trước ──────────────────────
    // Đọc lại giá trị hiện tại tại từng địa chỉ trong previous,
    // áp dụng operator (so với target hoặc so với previousValue),
    // trả về subset thỏa điều kiện.
    static std::vector<ScanResult> nextScan(
        HANDLE hProcess,
        const std::vector<ScanResult>& previous,
        const ScanValue& target,
        ScanOperator op
    );

    // ─── Đọc giá trị hiện tại tại 1 địa chỉ ────────────────────
    // Dùng để refresh giá trị hiển thị mỗi ~500ms trong DGV.
    // Trả về true nếu đọc thành công.
    static bool readValue(
        HANDLE hProcess,
        uint64_t address,
        ValueType type,
        ScanValue& outValue
    );

    // ─── Ghi giá trị mới vào 1 địa chỉ ─────────────────────────
    // Dùng cho tính năng "Edit value" / "Freeze".
    static bool writeValue(
        HANDLE hProcess,
        uint64_t address,
        const ScanValue& value
    );
};

} // namespace cheatvn

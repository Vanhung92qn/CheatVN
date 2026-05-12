#include "pch.h"
#include "MemoryScanner.h"

#include <cstring>     // memcpy
#include <algorithm>   // std::min, std::sort
#include <thread>      // std::thread, hardware_concurrency
#include <vector>
#include <immintrin.h> // AVX2 intrinsics: __m256i, _mm256_cmpeq_epi32,...

namespace cheatvn {

// =====================================================================
//  CHƯƠNG 1: enumerateRegions
//
//  Sử dụng VirtualQueryEx duyệt không gian địa chỉ ảo của process.
//
//  VirtualQueryEx hoạt động thế nào?
//  ──────────────────────────────────
//  Cho 1 địa chỉ A bất kỳ, OS trả về MEMORY_BASIC_INFORMATION cho biết
//  vùng nhớ liên tục chứa A: BaseAddress, RegionSize, State, Protect,...
//
//  Để duyệt toàn bộ:
//    addr = 0;
//    while (VirtualQueryEx(hProc, addr, &mbi, ...) != 0) {
//        // xử lý mbi...
//        addr = mbi.BaseAddress + mbi.RegionSize;
//    }
//
//  Mỗi lần lặp, đẩy con trỏ qua hết region hiện tại để query region kế.
//  Khi VirtualQueryEx trả 0 = đã đến cuối không gian địa chỉ.
//
//  Trên x64, không gian địa chỉ là 128 TB (47-bit). Process bình thường
//  chỉ chiếm vài chục MB → vài GB. Duyệt nhanh (~10-50ms).
// =====================================================================
std::vector<MemoryRegion> MemoryScanner::enumerateRegions(HANDLE hProcess) {
    std::vector<MemoryRegion> regions;
    regions.reserve(1024);  // Process trung bình ~1-3 nghìn region

    MEMORY_BASIC_INFORMATION mbi{};
    uint8_t* addr = nullptr;

    while (VirtualQueryEx(hProcess, addr, &mbi, sizeof(mbi)) == sizeof(mbi)) {
        // Chỉ giữ region đã COMMIT (đã có data thực).
        // MEM_RESERVE = đặt chỗ chưa có data → skip.
        // MEM_FREE = chưa được sử dụng → skip.
        if (mbi.State == MEM_COMMIT) {
            MemoryRegion r;
            r.baseAddress = reinterpret_cast<uint64_t>(mbi.BaseAddress);
            r.size = static_cast<uint64_t>(mbi.RegionSize);
            r.protection = mbi.Protect;
            r.state = mbi.State;
            r.type = mbi.Type;

            // Chỉ giữ region đọc được (skip PAGE_NOACCESS, PAGE_GUARD)
            if (r.isReadable()) {
                regions.push_back(r);
            }
        }

        // Tiến lên region kế tiếp
        uint8_t* next = (uint8_t*)mbi.BaseAddress + mbi.RegionSize;

        // Phòng tràn số (cuối không gian địa chỉ trên x64)
        if (next <= addr) break;
        addr = next;
    }

    return regions;
}

// =====================================================================
//  CHƯƠNG 2: Helpers cho scan
// =====================================================================

namespace {

    // Compile-time map từ C++ type → ValueType enum.
    // Dùng cho template scanRegion bên dưới: khi T=int32_t thì sinh
    // ValueType::Int32 vào ScanResult.
    template<typename T> struct TypeOf;
    template<> struct TypeOf<int8_t>   { static constexpr ValueType v = ValueType::Int8; };
    template<> struct TypeOf<int16_t>  { static constexpr ValueType v = ValueType::Int16; };
    template<> struct TypeOf<int32_t>  { static constexpr ValueType v = ValueType::Int32; };
    template<> struct TypeOf<int64_t>  { static constexpr ValueType v = ValueType::Int64; };
    template<> struct TypeOf<float>    { static constexpr ValueType v = ValueType::Float; };
    template<> struct TypeOf<double>   { static constexpr ValueType v = ValueType::Double; };

    // Extract giá trị type T từ ScanValue.
    template<typename T> T getValue(const ScanValue& sv);
    template<> int8_t   getValue<int8_t>  (const ScanValue& sv) { return sv.i8;  }
    template<> int16_t  getValue<int16_t> (const ScanValue& sv) { return sv.i16; }
    template<> int32_t  getValue<int32_t> (const ScanValue& sv) { return sv.i32; }
    template<> int64_t  getValue<int64_t> (const ScanValue& sv) { return sv.i64; }
    template<> float    getValue<float>   (const ScanValue& sv) { return sv.f32; }
    template<> double   getValue<double>  (const ScanValue& sv) { return sv.f64; }

    // Pack giá trị type T thành ScanValue.
    template<typename T> ScanValue makeValue(T v);
    template<> ScanValue makeValue<int8_t>  (int8_t v)   { return ScanValue::makeInt8(v);   }
    template<> ScanValue makeValue<int16_t> (int16_t v)  { return ScanValue::makeInt16(v);  }
    template<> ScanValue makeValue<int32_t> (int32_t v)  { return ScanValue::makeInt32(v);  }
    template<> ScanValue makeValue<int64_t> (int64_t v)  { return ScanValue::makeInt64(v);  }
    template<> ScanValue makeValue<float>   (float v)    { return ScanValue::makeFloat(v);  }
    template<> ScanValue makeValue<double>  (double v)   { return ScanValue::makeDouble(v); }

    // Predicate cho first-scan operator: so giữa value đọc được và target.
    // Inline để compiler tối ưu vào vòng lặp scan nóng.
    template<typename T>
    inline bool matchFirstScan(T value, T target, ScanOperator op) {
        switch (op) {
            case ScanOperator::Exact:          return value == target;
            case ScanOperator::NotEqual:       return value != target;
            case ScanOperator::Greater:        return value >  target;
            case ScanOperator::GreaterOrEqual: return value >= target;
            case ScanOperator::Less:           return value <  target;
            case ScanOperator::LessOrEqual:    return value <= target;
            default:                           return false; // các op còn lại không hợp lệ cho first
        }
    }

    // Predicate cho next-scan operator: so giữa current, previous, target.
    template<typename T>
    inline bool matchNextScan(T current, T previous, T target, ScanOperator op) {
        switch (op) {
            // So với target (giống first scan)
            case ScanOperator::Exact:          return current == target;
            case ScanOperator::NotEqual:       return current != target;
            case ScanOperator::Greater:        return current >  target;
            case ScanOperator::GreaterOrEqual: return current >= target;
            case ScanOperator::Less:           return current <  target;
            case ScanOperator::LessOrEqual:    return current <= target;

            // So với previous
            case ScanOperator::Changed:        return current != previous;
            case ScanOperator::Unchanged:      return current == previous;
            case ScanOperator::Increased:      return current >  previous;
            case ScanOperator::Decreased:      return current <  previous;
            case ScanOperator::IncreasedBy:    return (current - previous) == target;
            case ScanOperator::DecreasedBy:    return (previous - current) == target;

            default: return false;
        }
    }

    // ─── SIMD AVX2 scan cho Int32 ────────────────────────────────
    //
    // Xử lý 8 int32 (= 32 bytes) cùng lúc bằng 1 instruction AVX2.
    // Yêu cầu CPU support AVX2 (Intel Haswell 2013+, AMD Excavator 2015+).
    //
    // Algorithm:
    //   1. Broadcast target vào 1 ymm register (8 lanes giống nhau)
    //   2. Load 32 bytes data → ymm register
    //   3. Compare → ymm result (mỗi lane = 0 hoặc -1)
    //   4. movemask → int32 bitmask (mỗi lane chiếm 4 bit)
    //   5. Nếu mask != 0: scan 8 lane, lane nào set thì add address
    //
    // Tail (bytes < 32 cuối): xử lý bằng scalar.
    //
    // Operator được support qua SIMD:
    //   Exact, NotEqual, Greater, GreaterOrEqual, Less, LessOrEqual
    // Operator khác (Changed, Increased,...) → caller dùng scalar fallback.
    bool scanRegionInt32_AVX2(
        const uint8_t* buffer,
        size_t bytesRead,
        int32_t target,
        ScanOperator op,
        uint64_t baseAddress,
        std::vector<ScanResult>& results)
    {
        // Check operator có support SIMD không
        bool simdOk = (op == ScanOperator::Exact          ||
                       op == ScanOperator::NotEqual       ||
                       op == ScanOperator::Greater        ||
                       op == ScanOperator::GreaterOrEqual ||
                       op == ScanOperator::Less           ||
                       op == ScanOperator::LessOrEqual);
        if (!simdOk) return false;
        if (bytesRead < 32) return false;  // không đủ 1 chunk SIMD

        // Broadcast target vào 8 lane của 256-bit register
        __m256i targetVec = _mm256_set1_epi32(target);

        // Hằng số -1 cho mọi byte (dùng để invert kết quả)
        __m256i allOnes = _mm256_set1_epi8(-1);

        // Số chunk 32-byte hoàn chỉnh
        size_t chunks = bytesRead / 32;

        for (size_t c = 0; c < chunks; ++c) {
            size_t i = c * 32;

            // Load 32 bytes (8 int32) — loadu = unaligned load (an toàn)
            __m256i values = _mm256_loadu_si256(
                reinterpret_cast<const __m256i*>(buffer + i));

            // Tính cmp register theo operator
            __m256i cmp;
            switch (op) {
                case ScanOperator::Exact:
                    cmp = _mm256_cmpeq_epi32(values, targetVec);
                    break;
                case ScanOperator::NotEqual:
                    // not(equal) = xor với all-ones
                    cmp = _mm256_xor_si256(
                        _mm256_cmpeq_epi32(values, targetVec), allOnes);
                    break;
                case ScanOperator::Greater:
                    cmp = _mm256_cmpgt_epi32(values, targetVec);
                    break;
                case ScanOperator::Less:
                    // a < b ↔ b > a
                    cmp = _mm256_cmpgt_epi32(targetVec, values);
                    break;
                case ScanOperator::GreaterOrEqual:
                    // a >= b ↔ !(b > a) ↔ xor với all-ones
                    cmp = _mm256_xor_si256(
                        _mm256_cmpgt_epi32(targetVec, values), allOnes);
                    break;
                case ScanOperator::LessOrEqual:
                    // a <= b ↔ !(a > b)
                    cmp = _mm256_xor_si256(
                        _mm256_cmpgt_epi32(values, targetVec), allOnes);
                    break;
                default:
                    return false;
            }

            // movemask: lấy MSB của mỗi byte → 32-bit int.
            // Mỗi int32-lane chiếm 4 byte → 4 bit trong mask.
            // Mask=0 nghĩa là không lane nào match → fast skip.
            int mask = _mm256_movemask_epi8(cmp);
            if (mask == 0) continue;

            // Có ít nhất 1 lane match — duyệt 8 lane
            for (int lane = 0; lane < 8; ++lane) {
                // Lane k chiếm 4 bit ở vị trí (k*4 .. k*4+3)
                // Match nếu cả 4 bit này set (= 0xF)
                if ((mask >> (lane * 4)) & 0xF) {
                    ScanResult r;
                    r.address = baseAddress + i + lane * 4;
                    int32_t val;
                    std::memcpy(&val, buffer + i + lane * 4, sizeof(val));
                    r.currentValue = makeValue<int32_t>(val);
                    r.hasPrevious = false;
                    results.push_back(r);
                }
            }
        }

        // ─── Tail: bytes còn lại sau chunks * 32 ─────────────────
        // Dùng scalar cho phần này (max 31 bytes = max 7 int32)
        size_t tailStart = chunks * 32;
        if (tailStart + sizeof(int32_t) <= bytesRead) {
            size_t lastIdx = bytesRead - sizeof(int32_t);
            for (size_t i = tailStart; i <= lastIdx; i += 4) {
                int32_t val;
                std::memcpy(&val, buffer + i, sizeof(val));
                if (matchFirstScan<int32_t>(val, target, op)) {
                    ScanResult r;
                    r.address = baseAddress + i;
                    r.currentValue = makeValue<int32_t>(val);
                    r.hasPrevious = false;
                    results.push_back(r);
                }
            }
        }

        return true;  // SIMD đã xử lý xong region
    }

    // ─── Scan 1 region cho type T (template) ─────────────────────
    template<typename T>
    void scanRegionForType(
        HANDLE hProcess,
        const MemoryRegion& region,
        T target,
        ScanOperator op,
        bool aligned,
        std::vector<ScanResult>& results,
        std::vector<uint8_t>& buffer)
    {
        // Phòng region quá nhỏ
        if (region.size < sizeof(T)) return;

        // Cấp phát buffer đủ chứa region (tái sử dụng giữa các region)
        buffer.resize(static_cast<size_t>(region.size));

        SIZE_T bytesRead = 0;
        BOOL ok = ReadProcessMemory(
            hProcess,
            reinterpret_cast<LPCVOID>(region.baseAddress),
            buffer.data(),
            static_cast<SIZE_T>(region.size),
            &bytesRead
        );

        // Một số region có thể đọc partial (vd: chạm guard page) — vẫn xử lý
        // bytesRead đã đọc được. Nếu fail hoàn toàn → skip.
        if (!ok && bytesRead == 0) return;
        if (bytesRead < sizeof(T)) return;

        // Step = sizeof(T) nếu aligned (4x nhanh), = 1 nếu unaligned.
        const size_t step = aligned ? sizeof(T) : 1;
        const size_t lastIdx = bytesRead - sizeof(T);

        for (size_t i = 0; i <= lastIdx; i += step) {
            T value;
            // memcpy thay vì *(T*)(buf+i) để tránh unaligned access UB
            std::memcpy(&value, buffer.data() + i, sizeof(T));

            if (matchFirstScan(value, target, op)) {
                ScanResult r;
                r.address = region.baseAddress + i;
                r.currentValue = makeValue<T>(value);
                r.hasPrevious = false;
                results.push_back(r);
            }
        }
    }

    // ─── Read 1 value tại 1 địa chỉ theo type ────────────────────
    template<typename T>
    bool readTypedValue(HANDLE hProcess, uint64_t address, T& out) {
        SIZE_T bytesRead = 0;
        BOOL ok = ReadProcessMemory(
            hProcess,
            reinterpret_cast<LPCVOID>(address),
            &out,
            sizeof(T),
            &bytesRead
        );
        return ok && bytesRead == sizeof(T);
    }

    // Dispatcher: gọi scanRegionForType với T phù hợp với ValueType.
    // FAST PATH: Int32 + aligned + supported operator → dùng SIMD AVX2.
    void dispatchScan(
        HANDLE hProcess,
        const MemoryRegion& region,
        const ScanValue& target,
        ScanOperator op,
        bool aligned,
        std::vector<ScanResult>& results,
        std::vector<uint8_t>& buffer)
    {
        // ─── Optimization: SIMD path cho Int32 + aligned ──────────
        if (target.type == ValueType::Int32 && aligned) {
            // Đọc memory vào buffer
            buffer.resize(static_cast<size_t>(region.size));
            SIZE_T bytesRead = 0;
            BOOL ok = ReadProcessMemory(
                hProcess,
                reinterpret_cast<LPCVOID>(region.baseAddress),
                buffer.data(),
                static_cast<SIZE_T>(region.size),
                &bytesRead);
            if (!ok && bytesRead == 0) return;
            if (bytesRead < sizeof(int32_t)) return;

            // Thử SIMD trước. Nếu operator không support SIMD,
            // function trả false → fallback scalar bên dưới.
            if (scanRegionInt32_AVX2(buffer.data(), bytesRead,
                                     target.i32, op,
                                     region.baseAddress, results)) {
                return;  // SIMD xử lý xong
            }

            // SIMD không support op (vd: Changed) → scalar inline
            const size_t lastIdx = bytesRead - sizeof(int32_t);
            for (size_t i = 0; i <= lastIdx; i += sizeof(int32_t)) {
                int32_t value;
                std::memcpy(&value, buffer.data() + i, sizeof(value));
                if (matchFirstScan<int32_t>(value, target.i32, op)) {
                    ScanResult r;
                    r.address = region.baseAddress + i;
                    r.currentValue = makeValue<int32_t>(value);
                    r.hasPrevious = false;
                    results.push_back(r);
                }
            }
            return;
        }

        // ─── Scalar path cho các type còn lại ─────────────────────
        switch (target.type) {
            case ValueType::Int8:
                scanRegionForType<int8_t>(hProcess, region, target.i8, op, aligned, results, buffer); break;
            case ValueType::Int16:
                scanRegionForType<int16_t>(hProcess, region, target.i16, op, aligned, results, buffer); break;
            case ValueType::Int32:
                scanRegionForType<int32_t>(hProcess, region, target.i32, op, aligned, results, buffer); break;
            case ValueType::Int64:
                scanRegionForType<int64_t>(hProcess, region, target.i64, op, aligned, results, buffer); break;
            case ValueType::Float:
                scanRegionForType<float>(hProcess, region, target.f32, op, aligned, results, buffer); break;
            case ValueType::Double:
                scanRegionForType<double>(hProcess, region, target.f64, op, aligned, results, buffer); break;
            default:
                // String/ByteArray — chưa support
                break;
        }
    }

} // anonymous namespace

// =====================================================================
//  CHƯƠNG 3: firstScan — multi-threaded
//
//  Chiến lược song song:
//   1. enumerateRegions → list các vùng cần scan
//   2. Phân bổ region cho N thread theo greedy bin-packing
//      (sort theo size giảm dần, gán region lớn cho thread ít việc nhất)
//      → các thread có lượng byte tương đương → cân bằng tải
//   3. Mỗi thread chạy độc lập, append vào local vector (KHÔNG share state)
//   4. Khi xong, merge các local vector → kết quả chung
//
//  Tại sao KHÔNG dùng mutex/atomic share?
//  ──────────────────────────────────────
//  Mỗi lần push_back vào shared vector cần lock. Vài triệu push → triệu
//  lần lock = chậm. Local vector + merge cuối nhanh hơn nhiều.
//  Đây là pattern "embarrassingly parallel" — không có shared state.
//
//  Speedup thực tế: trên CPU 8 lõi, scan ~4-7x nhanh hơn single-thread.
//  Không lý tưởng (8x) vì memory bandwidth bottleneck (RAM throughput).
// =====================================================================
std::vector<ScanResult> MemoryScanner::firstScan(
    HANDLE hProcess,
    const ScanValue& target,
    ScanOperator op,
    const ScanOptions& opts)
{
    // ─── B1: enumerate + filter regions ─────────────────────────
    auto allRegions = enumerateRegions(hProcess);

    std::vector<MemoryRegion> regions;
    regions.reserve(allRegions.size());
    for (const auto& r : allRegions) {
        if (opts.skipImageRegions && r.isImage()) continue;
        if (opts.skipReadOnly && !r.isWritable()) continue;
        if (opts.maxRegionSize > 0 && r.size > opts.maxRegionSize) continue;
        regions.push_back(r);
    }

    if (regions.empty()) return {};

    // ─── B2: xác định số thread ─────────────────────────────────
    unsigned int nThreads = opts.numThreads;
    if (nThreads == 0) {
        nThreads = std::thread::hardware_concurrency();
        if (nThreads == 0) nThreads = 4;  // fallback nếu API fail
    }
    // Không hơn số region (mỗi thread cần ít nhất 1 region)
    if (nThreads > regions.size()) nThreads = (unsigned int)regions.size();
    if (nThreads < 1) nThreads = 1;

    // ─── B3: partition theo greedy bin-packing ──────────────────
    // Sort regions size DESCENDING — region to nhất xếp trước.
    std::sort(regions.begin(), regions.end(),
              [](const MemoryRegion& a, const MemoryRegion& b) {
                  return a.size > b.size;
              });

    std::vector<std::vector<MemoryRegion>> bins(nThreads);
    std::vector<uint64_t> binBytes(nThreads, 0);

    for (const auto& r : regions) {
        // Gán region vào bin có ít byte nhất hiện tại
        size_t minIdx = 0;
        for (size_t i = 1; i < nThreads; ++i) {
            if (binBytes[i] < binBytes[minIdx]) minIdx = i;
        }
        bins[minIdx].push_back(r);
        binBytes[minIdx] += r.size;
    }

    // ─── B4: launch threads, mỗi thread scan bin của nó ─────────
    std::vector<std::vector<ScanResult>> threadResults(nThreads);
    std::vector<std::thread> workers;
    workers.reserve(nThreads);

    for (unsigned int t = 0; t < nThreads; ++t) {
        // Capture by reference — các thread chạy đồng thời, không xung đột
        // vì mỗi thread chỉ truy cập threadResults[t] (slot riêng).
        workers.emplace_back([&, t]() {
            std::vector<uint8_t> buffer;  // local buffer per thread
            threadResults[t].reserve(1024);
            for (const auto& region : bins[t]) {
                dispatchScan(hProcess, region, target, op,
                             opts.aligned, threadResults[t], buffer);
            }
        });
    }

    // ─── B5: chờ tất cả thread xong ─────────────────────────────
    for (auto& w : workers) w.join();

    // ─── B6: merge các local vector ─────────────────────────────
    // Tính tổng để reserve trước → tránh nhiều lần grow vector.
    size_t totalCount = 0;
    for (const auto& tr : threadResults) totalCount += tr.size();

    std::vector<ScanResult> merged;
    merged.reserve(totalCount);
    for (auto& tr : threadResults) {
        // std::move iterator → di chuyển element thay vì copy (nhanh hơn cho
        // ScanResult chứa ScanValue/std::vector<uint8_t>).
        merged.insert(merged.end(),
                      std::make_move_iterator(tr.begin()),
                      std::make_move_iterator(tr.end()));
    }

    return merged;
}

// =====================================================================
//  CHƯƠNG 4: nextScan
// =====================================================================
//
//  nextScan KHÁC firstScan ở chỗ:
//   - Không enumerate region (đã có list địa chỉ từ first scan)
//   - Chỉ read value tại từng địa chỉ → so sánh với target/previous → keep/drop
//
//  → Nhanh hơn first scan rất nhiều (chỉ read N×sizeof(T) bytes thay vì
//     scan vài trăm MB).
//
//  Algorithm:
//   foreach result in previous:
//      read current value at result.address
//      if matchNextScan(current, previous.currentValue, target, op):
//         keep with previousValue = result.currentValue
//                     currentValue = current
//
template<typename T>
static void nextScanForType(
    HANDLE hProcess,
    const std::vector<ScanResult>& previous,
    T target,
    ScanOperator op,
    std::vector<ScanResult>& output)
{
    output.reserve(previous.size());

    for (const auto& prev : previous) {
        T current;
        if (!readTypedValue<T>(hProcess, prev.address, current)) {
            // Đọc fail → địa chỉ không còn valid (process re-allocated?)
            // Bỏ qua, không add vào output.
            continue;
        }

        T prevValue = getValue<T>(prev.currentValue);

        if (matchNextScan(current, prevValue, target, op)) {
            ScanResult r;
            r.address = prev.address;
            r.currentValue = makeValue<T>(current);
            r.previousValue = prev.currentValue;
            r.hasPrevious = true;
            output.push_back(r);
        }
    }
}

std::vector<ScanResult> MemoryScanner::nextScan(
    HANDLE hProcess,
    const std::vector<ScanResult>& previous,
    const ScanValue& target,
    ScanOperator op)
{
    std::vector<ScanResult> output;

    switch (target.type) {
        case ValueType::Int8:
            nextScanForType<int8_t>(hProcess, previous, target.i8, op, output); break;
        case ValueType::Int16:
            nextScanForType<int16_t>(hProcess, previous, target.i16, op, output); break;
        case ValueType::Int32:
            nextScanForType<int32_t>(hProcess, previous, target.i32, op, output); break;
        case ValueType::Int64:
            nextScanForType<int64_t>(hProcess, previous, target.i64, op, output); break;
        case ValueType::Float:
            nextScanForType<float>(hProcess, previous, target.f32, op, output); break;
        case ValueType::Double:
            nextScanForType<double>(hProcess, previous, target.f64, op, output); break;
        default: break;
    }

    return output;
}

// =====================================================================
//  CHƯƠNG 5: readValue / writeValue
// =====================================================================
bool MemoryScanner::readValue(
    HANDLE hProcess,
    uint64_t address,
    ValueType type,
    ScanValue& outValue)
{
    outValue.type = type;
    SIZE_T bytesRead = 0;

    size_t size = valueTypeSize(type);
    if (size == 0) return false;  // String/ByteArray chưa support

    // Đọc thẳng vào field tương ứng của union
    void* dest = nullptr;
    switch (type) {
        case ValueType::Int8:   dest = &outValue.i8;  break;
        case ValueType::Int16:  dest = &outValue.i16; break;
        case ValueType::Int32:  dest = &outValue.i32; break;
        case ValueType::Int64:  dest = &outValue.i64; break;
        case ValueType::Float:  dest = &outValue.f32; break;
        case ValueType::Double: dest = &outValue.f64; break;
        default: return false;
    }

    BOOL ok = ReadProcessMemory(
        hProcess,
        reinterpret_cast<LPCVOID>(address),
        dest,
        size,
        &bytesRead
    );
    return ok && bytesRead == size;
}

bool MemoryScanner::writeValue(
    HANDLE hProcess,
    uint64_t address,
    const ScanValue& value)
{
    SIZE_T bytesWritten = 0;
    size_t size = valueTypeSize(value.type);
    if (size == 0) return false;

    const void* src = nullptr;
    switch (value.type) {
        case ValueType::Int8:   src = &value.i8;  break;
        case ValueType::Int16:  src = &value.i16; break;
        case ValueType::Int32:  src = &value.i32; break;
        case ValueType::Int64:  src = &value.i64; break;
        case ValueType::Float:  src = &value.f32; break;
        case ValueType::Double: src = &value.f64; break;
        default: return false;
    }

    BOOL ok = WriteProcessMemory(
        hProcess,
        reinterpret_cast<LPVOID>(address),
        src,
        size,
        &bytesWritten
    );
    return ok && bytesWritten == size;
}

} // namespace cheatvn

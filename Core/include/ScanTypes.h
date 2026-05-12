#pragma once

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace cheatvn {

// =====================================================================
//  ValueType — loại dữ liệu user muốn quét trong memory.
//
//  Mỗi giá trị trong memory được decode theo 1 trong các kiểu này. VD:
//  cùng 4 bytes ở 1 địa chỉ có thể là Int32 (số nguyên) hoặc Float (số
//  thập phân) — đọc bằng cách interpret khác nhau.
// =====================================================================
enum class ValueType : uint8_t {
    Int8    = 0,  // 1 byte có dấu (-128 → 127)
    Int16   = 1,  // 2 bytes (-32k → 32k)
    Int32   = 2,  // 4 bytes — phổ biến nhất (HP, mana, tiền,...)
    Int64   = 3,  // 8 bytes (số rất lớn, big-money)
    Float   = 4,  // 4 bytes IEEE 754 (HP dạng thập phân, time-elapsed)
    Double  = 5,  // 8 bytes IEEE 754 (tọa độ, fine-grained float)
    String  = 6,  // chuỗi UTF-16 hoặc ASCII (tên nhân vật)
    ByteArray = 7 // dãy byte tùy ý (signature, pattern)
};

// Trả về kích thước byte của 1 ValueType cố định.
// (String/ByteArray = 0 vì kích thước biến thiên — phải hỏi riêng)
inline size_t valueTypeSize(ValueType t) {
    switch (t) {
        case ValueType::Int8:   return 1;
        case ValueType::Int16:  return 2;
        case ValueType::Int32:  return 4;
        case ValueType::Int64:  return 8;
        case ValueType::Float:  return 4;
        case ValueType::Double: return 8;
        default:                return 0;
    }
}

// =====================================================================
//  ScanOperator — phép so sánh khi quét.
//
//  - 6 phép đầu so với GIÁ TRỊ user nhập.
//  - 6 phép sau so với giá trị scan TRƯỚC ĐÓ (next scan only).
//
//  Đây chính là phần "thông minh" của Cheat Engine: scan 1 lần ra triệu
//  địa chỉ, scan tiếp với "Changed" lọc còn vài chục.
// =====================================================================
enum class ScanOperator : uint8_t {
    // ─ So với giá trị nhập ─
    Exact          = 0,  // = value
    NotEqual       = 1,  // != value
    Greater        = 2,  // > value
    GreaterOrEqual = 3,  // >= value
    Less           = 4,  // < value
    LessOrEqual    = 5,  // <= value
    Between        = 6,  // value1 <= x <= value2 (cần 2 operand)

    // ─ So với scan trước (chỉ dùng cho next scan) ─
    Changed        = 7,  // giá trị đã đổi
    Unchanged      = 8,  // giá trị giữ nguyên
    Increased      = 9,  // tăng so với lần trước
    Decreased      = 10, // giảm
    IncreasedBy    = 11, // tăng đúng N (vd: +5 máu)
    DecreasedBy    = 12  // giảm đúng N
};

// =====================================================================
//  ScanValue — chứa 1 giá trị cụ thể (với loại của nó).
//
//  Đây là pattern "tagged union" — 1 struct chứa data của nhiều type khác
//  nhau, kèm 1 field "type" để biết đang chứa loại gì. Đơn giản hơn dùng
//  std::variant (C++17) và tương thích với marshal C++/CLI dễ hơn.
//
//  Lưu ý: union không hold std::string/vector được (vì có constructor) →
//  ta để string/bytes ngoài union, dùng khi type là String/ByteArray.
// =====================================================================
struct ScanValue {
    ValueType type = ValueType::Int32;

    // Union của các kiểu cố định kích thước
    union {
        int8_t   i8;
        int16_t  i16;
        int32_t  i32;
        int64_t  i64;
        float    f32;
        double   f64;
    };

    // Cho String/ByteArray (không nằm trong union vì có destructor)
    std::vector<uint8_t> bytes;

    ScanValue() : i64(0) {}  // init union qua field lớn nhất

    // ─── Factory methods cho từng kiểu ───────────────────────────
    static ScanValue makeInt8(int8_t v)       { ScanValue r; r.type = ValueType::Int8;   r.i8 = v;  return r; }
    static ScanValue makeInt16(int16_t v)     { ScanValue r; r.type = ValueType::Int16;  r.i16 = v; return r; }
    static ScanValue makeInt32(int32_t v)     { ScanValue r; r.type = ValueType::Int32;  r.i32 = v; return r; }
    static ScanValue makeInt64(int64_t v)     { ScanValue r; r.type = ValueType::Int64;  r.i64 = v; return r; }
    static ScanValue makeFloat(float v)       { ScanValue r; r.type = ValueType::Float;  r.f32 = v; return r; }
    static ScanValue makeDouble(double v)     { ScanValue r; r.type = ValueType::Double; r.f64 = v; return r; }

    // Đọc giá trị từ raw memory buffer + interpret theo type.
    // Trả về ScanValue. Caller phải đảm bảo buffer có ít nhất valueTypeSize() bytes.
    static ScanValue fromBytes(const void* buffer, ValueType type) {
        ScanValue r;
        r.type = type;
        switch (type) {
            case ValueType::Int8:   r.i8  = *(const int8_t*)buffer;  break;
            case ValueType::Int16:  r.i16 = *(const int16_t*)buffer; break;
            case ValueType::Int32:  r.i32 = *(const int32_t*)buffer; break;
            case ValueType::Int64:  r.i64 = *(const int64_t*)buffer; break;
            case ValueType::Float:  r.f32 = *(const float*)buffer;   break;
            case ValueType::Double: r.f64 = *(const double*)buffer;  break;
            default: break;
        }
        return r;
    }

    // Format thành string để hiển thị (cho DataGridView).
    std::wstring toDisplay() const {
        wchar_t buf[64];
        switch (type) {
            case ValueType::Int8:   swprintf_s(buf, L"%d",  (int)i8);  return buf;
            case ValueType::Int16:  swprintf_s(buf, L"%d",  (int)i16); return buf;
            case ValueType::Int32:  swprintf_s(buf, L"%d",  i32);      return buf;
            case ValueType::Int64:  swprintf_s(buf, L"%lld", (long long)i64); return buf;
            case ValueType::Float:  swprintf_s(buf, L"%.4f", f32);     return buf;
            case ValueType::Double: swprintf_s(buf, L"%.6f", f64);     return buf;
            default: return L"?";
        }
    }
};

// =====================================================================
//  ScanResult — 1 địa chỉ tìm được + giá trị hiện tại và trước đó.
// =====================================================================
struct ScanResult {
    uint64_t  address = 0;            // Địa chỉ trong process target
    ScanValue currentValue;           // Giá trị hiện tại
    ScanValue previousValue;          // Giá trị trong lần scan trước (next scan)
    bool      hasPrevious = false;    // false sau firstScan, true sau nextScan
};

} // namespace cheatvn

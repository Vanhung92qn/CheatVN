#pragma once

#include <string>
#include <cstdint>

namespace cheatvn {

// Snapshot thông tin về 1 process tại thời điểm liệt kê.
// Đây là POD-like struct, chỉ chứa data — không có method ảo, dễ marshal.
struct ProcessInfo {
    uint32_t    pid          = 0;     // Process ID
    std::wstring name;                // Tên file exe (vd: "chrome.exe")
    std::wstring path;                // Full path (vd: "C:\Program Files\...")
    bool        is64Bit      = false; // true = x64, false = x86 (WOW64)
    uint64_t    workingSetKB = 0;     // RAM đang dùng (KB)
    bool        isAccessible = true;  // false = process system bảo vệ, không OpenProcess được
};

} // namespace cheatvn

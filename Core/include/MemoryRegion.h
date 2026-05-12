#pragma once

#include <cstdint>
#include <vector>
#include <Windows.h>

namespace cheatvn {

// =====================================================================
//  MemoryRegion — đại diện 1 vùng nhớ "đồng nhất" trong process.
//
//  Process khi chạy KHÔNG có 1 khối memory liên tục. OS chia memory ảo
//  thành nhiều "region" — mỗi region có:
//    - cùng protection (PAGE_READWRITE, PAGE_READONLY, PAGE_NOACCESS,...)
//    - cùng state (MEM_COMMIT đã cấp phát, MEM_RESERVE chỉ reserve,
//                  MEM_FREE chưa dùng)
//    - cùng type (MEM_PRIVATE heap, MEM_IMAGE exe/dll loaded, MEM_MAPPED
//                 file-mapping)
//
//  Mục đích của struct này:
//  ─────────────────────────
//  Khi scan memory, ta KHÔNG đọc cả vùng địa chỉ 0 → 2^64. Mà ta dùng
//  VirtualQueryEx để hỏi OS xem vùng nào ĐANG CÓ data (MEM_COMMIT), rồi
//  chỉ đọc các vùng đó. Tiết kiệm 99.99% thời gian.
//
//  VD process Notepad có ~3 nghìn region. Chỉ scan ~1500 region readable.
// =====================================================================
struct MemoryRegion {
    // Địa chỉ bắt đầu (trong không gian địa chỉ ảo của process target)
    uint64_t baseAddress = 0;

    // Kích thước vùng (bytes)
    uint64_t size = 0;

    // Cờ bảo vệ: PAGE_READWRITE / PAGE_READONLY / PAGE_EXECUTE_READ ...
    // Dùng để check có đọc/ghi được không.
    uint32_t protection = 0;

    // Trạng thái: MEM_COMMIT / MEM_RESERVE / MEM_FREE
    uint32_t state = 0;

    // Loại: MEM_PRIVATE (heap/stack) / MEM_IMAGE (exe, dll) / MEM_MAPPED
    uint32_t type = 0;

    // ─── Helper inline ────────────────────────────────────────────

    // Có thể đọc memory này không? (mọi protect khác PAGE_NOACCESS / PAGE_GUARD)
    bool isReadable() const {
        if (protection & PAGE_NOACCESS) return false;
        if (protection & PAGE_GUARD)    return false;
        // Các protect cho phép đọc:
        constexpr uint32_t READABLE =
            PAGE_READONLY | PAGE_READWRITE | PAGE_WRITECOPY |
            PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY;
        return (protection & READABLE) != 0;
    }

    // Có thể ghi memory này không?
    bool isWritable() const {
        if (protection & PAGE_NOACCESS) return false;
        if (protection & PAGE_GUARD)    return false;
        constexpr uint32_t WRITABLE =
            PAGE_READWRITE | PAGE_WRITECOPY |
            PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY;
        return (protection & WRITABLE) != 0;
    }

    // Đây có phải code (exe/dll loaded) không?
    // Khi cần tránh scan code segments để giảm noise.
    bool isImage() const {
        return type == MEM_IMAGE;
    }

    // Heap/stack/private allocation?
    bool isPrivate() const {
        return type == MEM_PRIVATE;
    }
};

} // namespace cheatvn

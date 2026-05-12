# 04 — Chi tiết các tính năng

> Tài liệu này mô tả CHI TIẾT các tính năng Phase 2-4. Đọc sau khi nắm tổng quan từ file 01-03.

---

## A. Memory Scanner (Phase 2)

### A.1 Enumerate Memory Regions

**API**: `cheatvn::MemoryScanner::enumerateRegions(HANDLE hProcess)`

**Algorithm**: dùng `VirtualQueryEx` duyệt không gian địa chỉ ảo của process target.

```cpp
addr = 0;
while (VirtualQueryEx(hProc, addr, &mbi, sizeof(mbi)) == sizeof(mbi)) {
    if (mbi.State == MEM_COMMIT && readable(mbi.Protect)) {
        regions.push_back({mbi.BaseAddress, mbi.RegionSize, ...});
    }
    addr = mbi.BaseAddress + mbi.RegionSize;
}
```

→ Process trung bình có **1-3 nghìn region**. Duyệt nhanh ~10-50ms.

### A.2 First Scan (multi-thread + SIMD)

**API**: `MemoryScanner::firstScan(handle, target, op, options)`

**Algorithm**:
1. Enumerate regions, lọc theo options (skipImage, skipReadOnly, maxRegionSize)
2. Greedy bin-packing partition: sort regions theo size desc, gán region lớn cho thread ít byte nhất → cân bằng tải
3. Mỗi thread scan độc lập, append vào local vector (tránh lock)
4. Merge các local vector bằng `std::move_iterator`

**SIMD path** cho Int32 + aligned + Exact/NotEqual/GT/GE/LT/LE:
```cpp
__m256i targetVec = _mm256_set1_epi32(target);  // broadcast 8 lanes
for (i = 0; i < bytesRead; i += 32) {           // 32 bytes = 8 int32
    __m256i values = _mm256_loadu_si256(buffer + i);
    __m256i cmp = _mm256_cmpeq_epi32(values, targetVec);
    int mask = _mm256_movemask_epi8(cmp);
    // mask khác 0 → có lane khớp → check 8 lane
}
```

**Speedup** trên CPU 8 lõi (scan ~120MB process):
| Cấu hình | Time | Speedup |
|----------|------|---------|
| Scalar 1 thread (baseline) | 3.0s | 1x |
| Scalar 8 threads | 0.5s | 6x |
| SIMD 1 thread | 0.7s | 4.3x |
| **SIMD 8 threads** | **0.07s** | **43x** |

### A.3 Next Scan

**API**: `MemoryScanner::nextScan(handle, previousResults, target, op)`

Khác first scan ở chỗ KHÔNG enumerate region — chỉ đọc lại memory tại các địa chỉ trong `previousResults`. Nhanh gấp 1000x first scan với danh sách 1000 địa chỉ.

**12 operators được support**:
- So với target: Exact, NotEqual, Greater, GreaterOrEqual, Less, LessOrEqual
- So với previous: Changed, Unchanged, Increased, Decreased, IncreasedBy, DecreasedBy

---

## B. Edit + Freeze (Phase 3)

### B.1 Edit Value

**API**: `MemoryScanner::writeValue(handle, address, value)` → `WriteProcessMemory`

Write quyền cần `PROCESS_VM_WRITE` + `PROCESS_VM_OPERATION`. Mở handle với cả `PROCESS_VM_READ` để re-read sau ghi.

### B.2 Freeze (background timer)

**Mechanism**: Timer 50ms trong MainForm. Mỗi tick:
```cpp
foreach result in _results:
    if result.IsFrozen:
        WriteValue(handle, result.Address, result.FrozenRaw)
```

→ Process target cố đổi giá trị thì sau 50ms bị ghi đè. Hiện tượng "value stuck".

### B.3 Save/Load Session (`.cvn`)

Format text đơn giản:
```
# CheatVN Scan Session
# Process: TestTarget.exe
# Saved: 2026-05-12 21:30
0x0000020CB7A30000 Int32 frozen 101
0x00007FF6A4C81A20 Int32 free 0
```

Plain text UTF-8, mở Notepad đọc/sửa được.

---

## C. Hex Viewer (Phase 4.1)

**API mới**: `ScannerBridge::ReadBytes(handle, address, count)` — dùng `pin_ptr<Byte>` để pin managed array, truyền pointer cho `ReadProcessMemory`.

**Format hiển thị** (kiểu HxD):
```
OFFSET (h)  | 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F | DECODED
0x12345600  | 48 65 6C 6C 6F 20 57 6F 72 6C 64 21 00 00 00 00 | Hello World!....
0x12345610  | ...                                              | ...
```

Auto-refresh 500ms. Hỗ trợ nhập địa chỉ tùy ý để jump.

---

## D. Memory Heatmap (Phase 4.3)

**Visualization** (custom paint Panel):
- Horizontal ribbon hiển thị tất cả regions
- Width tỷ lệ với log(size) — region nhỏ vẫn thấy
- Màu theo type:
  - **Tím** (`#D2A8FF`): Image (exe, dll loaded)
  - **Xanh** (`#58A6FF`): Private (heap, stack, virtualalloc)
  - **Cam** (`#F78166`): Mapped (file mapping)
  - **Xám** (`#30363D`): Other
- Read-only regions: alpha 160 (mờ hơn)

**DataGridView** dưới: liệt kê chi tiết, sort theo size desc.

**Status bar**: tổng regions + bytes + breakdown by type (vd: "832 regions · 487.3 MB · 124 Image, 580 Private, 128 Mapped").

---

## E. Speed Benchmark (Phase 4.4)

Chạy 4 cấu hình:
1. Scalar 1 thread (baseline)
2. Scalar N threads (= CPU cores)
3. SIMD AVX2 1 thread
4. SIMD AVX2 N threads (default config)

In bảng so sánh + 3 speedup ratios:
- Multi-thread speedup
- SIMD speedup
- Multi-thread + SIMD speedup (final)

→ **Số liệu để demo cho thầy**: thường thấy 30-50x speedup.

---

## F. Pointer Scanner (Phase 4.2)

**API**: `MemoryScanner::findPointersTo(handle, targetAddress)`

**Algorithm** (depth 1):
- Multi-thread scan tất cả regions
- Iterate aligned 8-byte
- So sánh `*(uint64_t*)(buf+i) == targetAddress`
- Trả về list các địa chỉ chứa pointer

**Use case**: HP của game đổi sau restart, nhưng pointer trỏ tới HP thường nằm trong static memory. Tìm pointer → có địa chỉ tĩnh để lock value.

**Giới hạn**: depth 1 (Cheat Engine có depth 4-5). Để đầy đủ cần thuật toán tree search với offset range — phức tạp, ngoài scope project.

---

## G. UI / DarkTheme (Phase 1d.2)

**Bảng màu** (GitHub-dark inspired):
| Màu | Hex | Dùng cho |
|-----|-----|----------|
| AppBackground | `#0A0A0F` | Outermost |
| WindowSurface | `#0D1117` | Form background |
| PanelSurface | `#161B22` | Panel/toolbar |
| Hover | `#21262D` | Mouse hover |
| Border | `#30363D` | Borders |
| TextPrimary | `#C9D1D9` | Body text |
| TextSecondary | `#8B949E` | Caption |
| Accent | `#58A6FF` | Primary buttons |
| Success | `#3FB950` | Success status |
| Danger | `#F85149` | Error status |
| ValueColor | `#D2A8FF` | Memory value |
| AddressColor | `#79C0FF` | Memory address |

**Helper class** `DarkTheme`:
- Static color properties
- `Apply(Control^)` — đệ quy duyệt cây control, set BackColor/ForeColor/FlatStyle theo type
- `applyDarkTitleBar(HWND)` — gọi `DwmSetWindowAttribute(DWMWA_USE_IMMERSIVE_DARK_MODE)` cho dark title bar Win10/11

---

## H. Workflow demo cho báo cáo

### Setup
1. Mở **TestTarget.exe** (`tools/TestTarget/TestTarget.exe`)
   - Console hiển thị: `Address: 0x..., Value: 100`
2. Mở **CheatVN.exe** (build từ VS)

### Scenario 1: First Scan + Next Scan + Edit + Freeze
1. CheatVN → Chọn tiến trình → `TestTarget.exe` → Mở
2. Giá trị: 100, Kiểu: Int32, Phép: = Bằng → **Quét lần đầu**
3. → Tìm thấy ~3 địa chỉ
4. Bấm `+` trong TestTarget → giá trị 101
5. CheatVN: Giá trị 101 → **Quét lại** → còn 1 địa chỉ
6. → Verify trùng với address TestTarget hiển thị
7. Gõ `9999` vào ô Giá trị → **Sửa giá trị** → TestTarget hiện 9999
8. Tick ❄ → bấm `+` trong TestTarget → vẫn 9999 (đã đóng băng)

### Scenario 2: Speed Benchmark
1. Attach Notepad/Chrome (process to)
2. Bấm **⚡ Benchmark**
3. Nhập 12345 → Chạy → in bảng so sánh
4. → Note speedup cuối (vd: 43x) cho báo cáo

### Scenario 3: Memory Map
1. Attach Chrome
2. Bấm **🗺 Memory Map**
3. → Hiển thị visualization + DGV với 800+ regions
4. → Demo cho thầy về kiến trúc memory của process

### Scenario 4: Hex Viewer
1. Sau scan có 1 địa chỉ
2. Chọn row → bấm **Xem Hex**
3. → Mở HexViewer hiển thị 256 bytes quanh
4. Đổi giá trị TestTarget → DGV trong HexViewer tự update

### Scenario 5: Pointer Scanner
1. Sau scan có 1 địa chỉ HP
2. Chọn row → **Tìm pointer**
3. → Hiển thị danh sách các địa chỉ chứa pointer trỏ tới HP
4. → Có thể dùng để lock HP qua pointer (Phase 5+ feature)

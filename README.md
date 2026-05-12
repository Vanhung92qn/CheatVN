# CheatVN — Trình phân tích bộ nhớ tiến trình

> Đồ án tốt nghiệp / cuối kỳ — công cụ phân tích & can thiệp bộ nhớ kiểu Cheat Engine, viết bằng C++ thuần (Core) + C++/CLI Windows Forms (GUI), tối ưu multi-thread và SIMD AVX2.

![Stack](https://img.shields.io/badge/C%2B%2B-17-blue) ![CLI](https://img.shields.io/badge/C%2B%2B%2FCLI-NET%204.8-green) ![x64](https://img.shields.io/badge/x64-AVX2-orange)

---

## Tính năng

### Cốt lõi (Phase 1-3)
- **Process picker**: liệt kê tất cả tiến trình đang chạy (PID, tên, bitness, RAM), tìm kiếm + lọc, auto-refresh 2s
- **Memory Scanner**:
  - Quét 6 kiểu dữ liệu: `Int8/16/32/64`, `Float`, `Double`
  - 12 phép so sánh: Exact, NotEqual, Greater/Less (≥/≤), Changed/Unchanged, Increased/Decreased, IncreasedBy/DecreasedBy
  - First scan toàn bộ memory + Next scan filter
- **Sửa giá trị** (`WriteProcessMemory`): ghi giá trị mới vào địa chỉ đang chọn
- **Đóng băng (Freeze)**: background timer 50ms ghi lại giá trị liên tục → khóa cứng giá trị
- **Auto-refresh 500ms**: cột "Giá trị hiện tại" tự cập nhật theo memory thật
- **Save/Load session** (`.cvn`): lưu danh sách địa chỉ + frozen state ra text file

### Tối ưu hiệu năng (Phase 2h-i)
- **Multi-thread scanner**: greedy bin-packing partition (LPT scheduling), per-thread local vector tránh lock contention
- **SIMD AVX2**: scan 8 int32 cùng lúc với `_mm256_cmpeq_epi32` / `_mm256_cmpgt_epi32`
- **Speedup tổng**: scalar 1-thread → SIMD multi-thread = **30-40x** trên CPU 8 lõi

### Nâng cao (Phase 4)
- **Hex Viewer**: xem 256 bytes raw quanh 1 địa chỉ, format ADDRESS | HEX | ASCII (kiểu HxD), auto-refresh
- **Memory Heatmap**: visualize tất cả memory regions của process — ribbon màu theo type (image/private/mapped) + DataGridView chi tiết
- **Speed Benchmark**: chạy 4 cấu hình (scalar 1T, scalar NT, SIMD 1T, SIMD NT), in bảng so sánh + tóm tắt speedup
- **Pointer Scanner** (depth 1): tìm các địa chỉ chứa pointer trỏ tới target, đa luồng

### UI
- **Dark theme** custom (palette GitHub-dark) — không dùng NuGet package
- **DwmSetWindowAttribute**: title bar dark trên Windows 10/11
- Custom paint button với gradient + hover effect
- Tất cả label/text tiếng Việt, font Segoe UI; địa chỉ + giá trị memory dùng Consolas monospace

---

## Kiến trúc

```
┌─────────────────────────────────────────────────────────────┐
│  GUI  (project "GUI", C++/CLI Windows Forms, .NET 4.8)      │
│  ─────────────────────────────────────────────────────       │
│  MainForm, ProcessListForm, HexViewerForm,                   │
│  HeatmapForm, BenchmarkForm, PointerScannerForm              │
│  DarkTheme helper, ProcessManagerBridge, ScannerBridge       │
└─────────────────────────────────────────────────────────────┘
                              ⇅ marshal qua Bridge
┌─────────────────────────────────────────────────────────────┐
│  CORE  (project "Core", native C++ static library)           │
│  ─────────────────────────────────────────────────────       │
│  ProcessManager (Toolhelp32, OpenProcess, SeDebugPrivilege)  │
│  MemoryScanner (VirtualQueryEx, ReadProcessMemory,           │
│                  multi-thread, AVX2 SIMD)                    │
└─────────────────────────────────────────────────────────────┘
                              ⇅ Win32 API
┌─────────────────────────────────────────────────────────────┐
│  Windows kernel (NTDLL, kernel32, advapi32)                  │
└─────────────────────────────────────────────────────────────┘
```

Đọc chi tiết:
- [docs/01_TONG_QUAN.md](docs/01_TONG_QUAN.md) — Mục tiêu + kiến trúc 2-layer
- [docs/02_KIEN_THUC_NEN.md](docs/02_KIEN_THUC_NEN.md) — VS, Win32, C++/CLI
- [docs/03_GIAI_THICH_CODE.md](docs/03_GIAI_THICH_CODE.md) — Walk-through code
- [docs/04_TINH_NANG.md](docs/04_TINH_NANG.md) — Chi tiết các tính năng Phase 2-4

---

## Build từ source

### Yêu cầu
- **Visual Studio 2022** (Community OK) với:
  - Workload "Desktop development with C++"
  - Workload ".NET desktop development"
  - Individual component "C++/CLI support for v143 build tools"
- **Windows 10/11 x64**
- **CPU hỗ trợ AVX2** (Intel Haswell 2013+, AMD Excavator 2015+)
- **.NET Framework 4.8**

### Steps
1. Clone: `git clone https://github.com/Vanhung92qn/CheatVN.git`
2. Mở `CheatVN.sln` bằng VS 2022
3. Đặt platform = **x64** + configuration = **Debug** (hoặc Release)
4. Build solution (`Ctrl+Shift+B`)
5. Run (`F5`)

### Build TestTarget (app demo)
```
cd tools\TestTarget
build.bat       # chạy trong "Developer Command Prompt for VS 2022"
TestTarget.exe  # mở console app demo
```

---

## Demo workflow

### 1. Demo cơ bản (TestTarget)

```
1. Mở TestTarget.exe → console hiện "Address: 0x...   Value: 100"
2. Mở CheatVN → Chọn tiến trình → TestTarget.exe → Mở
3. Giá trị: 100, Kiểu: Int32, Phép: = Bằng → Quét lần đầu
4. Bấm + trong TestTarget → giá trị thành 101
5. Trong CheatVN: giá trị 101, Quét lại → còn 1 địa chỉ
6. Verify: địa chỉ trùng với address TestTarget hiển thị
7. Magic: gõ 9999 vào ô Giá trị → Sửa giá trị → TestTarget hiện 9999
8. Tick ❄ Đóng băng → bấm + trong TestTarget → giá trị bị "khóa" ở 9999
```

### 2. Demo Speed Benchmark
```
1. Attach process bất kỳ (notepad, chrome,...)
2. Bấm "⚡ Benchmark" trong toolbar
3. Nhập giá trị Int32 lạ (vd: 12345)
4. Bấm "Chạy benchmark"
5. → Kết quả: Scalar 1T 3.0s → Multi-thread + SIMD 0.07s = 43x speedup
```

### 3. Demo Memory Heatmap
```
1. Attach process Chrome (RAM 500MB+)
2. Bấm "🗺 Memory Map"
3. → Visualization ribbon các regions theo type
4. → DataGridView 800+ regions, sort theo size
```

---

## Cấu trúc project

```
CheatVN/
├── CheatVN.sln
├── Core/                       (native C++ static library)
│   ├── include/
│   │   ├── ProcessInfo.h       — struct mô tả 1 process
│   │   ├── ProcessManager.h    — enum/open/close process handle
│   │   ├── MemoryRegion.h      — struct vùng nhớ + helpers
│   │   ├── ScanTypes.h         — ValueType, ScanOperator, ScanValue, ScanResult
│   │   └── MemoryScanner.h     — first/next scan + read/write + pointer scan
│   └── src/
│       ├── ProcessManager.cpp
│       └── MemoryScanner.cpp   — multi-thread + AVX2 SIMD
│
├── GUI/                        (C++/CLI Windows Forms App)
│   ├── Program.cpp             — main() entry point
│   ├── MainForm.h              — cửa sổ chính
│   ├── ProcessListForm.h       — dialog chọn process
│   ├── HexViewerForm.h         — xem hex bytes
│   ├── HeatmapForm.h           — visualize memory regions
│   ├── BenchmarkForm.h         — so sánh scalar vs SIMD
│   ├── PointerScannerForm.h    — tìm pointer
│   ├── DarkTheme.h/.cpp        — theme helper
│   ├── ProcessManagerBridge.h/.cpp — bridge native↔managed cho process
│   └── ScannerBridge.h/.cpp    — bridge cho scanner
│
├── tools/
│   └── TestTarget/             (console app demo target)
│       ├── TestTarget.cpp
│       └── build.bat
│
├── docs/                       (tài liệu tiếng Việt)
│   ├── 01_TONG_QUAN.md
│   ├── 02_KIEN_THUC_NEN.md
│   ├── 03_GIAI_THICH_CODE.md
│   └── 04_TINH_NANG.md
│
└── mockups/                    (HTML mockups thiết kế UI)
    ├── 01_process_list.html
    └── 02_main_scanner.html
```

---

## Khuyến cáo & giới hạn

- **Mục đích học thuật**: project phục vụ học hệ điều hành / Win32 API / tối ưu hiệu năng C++. KHÔNG dùng để cheat game online (vi phạm ToS, có thể bị ban).
- **Yêu cầu Administrator** để mở handle vào nhiều process. Chạy "as Administrator" hoặc bật `SeDebugPrivilege`.
- **Chỉ x64** (không scan được process x86 từ tool x64).
- **Pointer Scanner** chỉ depth 1 (CE có depth 4-5).
- **String/Byte array scan** chưa support trong baseline.

---

## License

MIT (đồ án học tập).

## Tác giả

Vanhung92qn — đồ án 2026.

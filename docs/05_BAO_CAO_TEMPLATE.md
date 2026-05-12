# 05 — Template Báo cáo Đồ án

> Skeleton/outline cho báo cáo đồ án CheatVN. Bạn copy nội dung này sang Word/PDF, điền thêm hình + mô tả chi tiết.

---

## TRANG BÌA

```
ĐẠI HỌC [TÊN TRƯỜNG]
KHOA [TÊN KHOA]

ĐỒ ÁN [MÔN HỌC / TỐT NGHIỆP]

CHEATVN — XÂY DỰNG CÔNG CỤ PHÂN TÍCH
VÀ CAN THIỆP BỘ NHỚ TIẾN TRÌNH WINDOWS
ÁP DỤNG TỐI ƯU MULTI-THREAD VÀ SIMD AVX2

Sinh viên thực hiện: [Họ tên]   MSSV: [...]
Giảng viên hướng dẫn: [...]
Năm: 2026
```

---

## MỤC LỤC

1. Mở đầu
2. Cơ sở lý thuyết
3. Thiết kế hệ thống
4. Cài đặt & triển khai
5. Tối ưu hiệu năng
6. Kết quả thực nghiệm
7. Kết luận & hướng phát triển
8. Tài liệu tham khảo

---

## CHƯƠNG 1 — MỞ ĐẦU

### 1.1 Lý do chọn đề tài
- Nhu cầu hiểu sâu về hệ điều hành Windows: process, virtual memory, access control
- Cheat Engine là công cụ tham chiếu nhưng phức tạp; viết lại từ đầu giúp nắm được "bản chất cốt lõi"
- Phục vụ học tập về reverse engineering, security research, không nhằm mục đích phá game online

### 1.2 Mục tiêu đồ án
- Xây dựng tool **CheatVN** với các chức năng Cheat Engine cốt lõi
- Áp dụng tối ưu **multi-thread + SIMD AVX2** để show off sức mạnh C++
- Kiến trúc **2-layer (native Core + managed GUI)** sạch sẽ, dễ maintain
- UI **dark theme** custom paint (không dùng thư viện ngoài)

### 1.3 Phạm vi
- **In scope**: process picker, memory scanner (6 types, 12 operators), edit/freeze value, hex viewer, memory heatmap, pointer scanner depth 1, speed benchmark
- **Out of scope**: pointer scan depth 4-5, code injection, debugger, disassembler, anti-detection

### 1.4 Cấu trúc báo cáo
[Liệt kê 7 chương]

---

## CHƯƠNG 2 — CƠ SỞ LÝ THUYẾT

### 2.1 Process & Virtual Memory trong Windows
- Process là gì, các thành phần (PID, threads, handles, access token)
- Virtual address space: 128TB trên x64
- Memory regions: MEM_COMMIT/RESERVE/FREE, page protection
- WoW64: process 32-bit chạy trên Windows 64-bit

### 2.2 Win32 API liên quan
- `OpenProcess` + access rights (PROCESS_VM_READ, PROCESS_VM_WRITE, PROCESS_QUERY_INFORMATION)
- `VirtualQueryEx`: lấy info từng vùng nhớ
- `ReadProcessMemory` / `WriteProcessMemory`: cross-process memory I/O
- `CreateToolhelp32Snapshot`: liệt kê processes
- `OpenProcessToken` + `AdjustTokenPrivileges`: bật `SeDebugPrivilege`
- `IsWow64Process`: phát hiện bitness
- `DwmSetWindowAttribute`: dark title bar Windows 10/11

### 2.3 C++/CLI Bridge native ↔ managed
- Managed code (.NET) vs native code (Win32)
- `ref class` + `gcnew` + `^` (handle to managed object)
- Marshalling: `std::wstring` ↔ `String^`, `std::vector<T>` ↔ `List<T>^`
- `pin_ptr` để truyền managed array vào native function
- Chi phí transition managed↔native (~50ns mỗi lần)

### 2.4 Multi-threading C++
- `std::thread`, `hardware_concurrency()`
- Pattern "embarrassingly parallel" — chia data, mỗi thread xử lý độc lập
- Greedy bin-packing (LPT scheduling) — cân bằng tải
- Memory bandwidth bottleneck (RAM throughput)

### 2.5 SIMD AVX2
- SIMD: Single Instruction Multiple Data
- AVX2: 256-bit registers, 8 int32 cùng lúc
- Intrinsics: `_mm256_loadu_si256`, `_mm256_cmpeq_epi32`, `_mm256_movemask_epi8`
- Speedup theoretical 8x, thực tế ~4-6x (memory bound)

---

## CHƯƠNG 3 — THIẾT KẾ HỆ THỐNG

### 3.1 Kiến trúc tổng thể
[Đính kèm sơ đồ 3-layer: GUI / Bridge / Core / Win32 kernel]

### 3.2 Module Core (native C++)
- **ProcessManager**: enum/open/close process
- **MemoryRegion**: struct mô tả 1 vùng nhớ
- **ScanTypes**: enum ValueType, ScanOperator, struct ScanValue/ScanResult
- **MemoryScanner**: enumerateRegions, firstScan (multi-thread + SIMD), nextScan, readValue, writeValue, findPointersTo

### 3.3 Module GUI (C++/CLI)
- **MainForm**: cửa sổ chính, scan workflow
- **ProcessListForm**: dialog chọn process
- **HexViewerForm**: xem hex bytes
- **HeatmapForm**: visualize memory regions
- **BenchmarkForm**: so sánh hiệu năng
- **PointerScannerForm**: tìm pointer
- **DarkTheme**: helper áp dark theme
- **ProcessManagerBridge / ScannerBridge**: bridge layer

### 3.4 Database / file format
- Session file `.cvn`: text plain UTF-8, mỗi dòng 1 địa chỉ
- Format: `ADDRESS TYPE FROZEN FROZEN_VALUE`

### 3.5 Mockup UI
[Đính kèm screenshot mockups/01_process_list.html, 02_main_scanner.html]

---

## CHƯƠNG 4 — CÀI ĐẶT & TRIỂN KHAI

### 4.1 Môi trường phát triển
- Visual Studio 2022 Community
- C++/CLI support v143
- .NET Framework 4.8
- Target: Windows 10/11 x64

### 4.2 Cấu trúc project
[Liệt kê file structure đã có ở README]

### 4.3 Build & deploy
- Configuration: x64 Debug + Release
- Output: `x64/Debug/GUI.exe` + `x64/Debug/Core.lib`
- Yêu cầu CPU AVX2 (Haswell 2013+)

### 4.4 Workflow git
- Repository: https://github.com/Vanhung92qn/CheatVN
- Commits theo phase: Phase 1 → Phase 2a-i → Phase 3.1-3.3 → Phase 4.1-4.4 → Phase 5

---

## CHƯƠNG 5 — TỐI ƯU HIỆU NĂNG

### 5.1 Single-thread baseline
- `firstScan` đơn luồng quét aligned, predicate-based
- Speedup nền tảng cho so sánh

### 5.2 Multi-threading
- Thuật toán greedy bin-packing (LPT)
- Per-thread local vector, merge cuối với `std::move_iterator`
- KHÔNG dùng mutex → không có lock contention
- [Đính kèm sơ đồ partition + merge]

### 5.3 SIMD AVX2
- Intrinsics breakdown
- Code snippet `scanRegionInt32_AVX2`
- Tail handling (bytes < 32 cuối) bằng scalar fallback

### 5.4 Layered optimization
- Aligned scan (4x cho int32)
- Buffer tái sử dụng giữa regions
- Filter regions trước khi scan (skipImage, skipReadOnly, maxRegionSize)

---

## CHƯƠNG 6 — KẾT QUẢ THỰC NGHIỆM

### 6.1 Môi trường test
- CPU: [tên CPU + số core]
- RAM: [GB]
- OS: Windows 11 x64
- Process target: Notepad (~120MB), Chrome (~500MB), TestTarget (~1MB)

### 6.2 Bảng so sánh hiệu năng
[Chạy Benchmark Form, screenshot kết quả, nhập vào bảng]

| Process | Scalar 1T | Scalar NT | SIMD 1T | SIMD NT | Speedup |
|---------|-----------|-----------|---------|---------|---------|
| Notepad 120MB | 3.0s | 0.5s | 0.7s | 0.07s | 43x |
| Chrome 500MB | 12s | 1.5s | 2.8s | 0.4s | 30x |

### 6.3 Workflow demo
[Đính kèm screenshot từng bước scenario 1-5 từ docs/04_TINH_NANG.md]

### 6.4 Memory Heatmap analysis
- Notepad: ~150 regions, chủ yếu Image
- Chrome: ~3000 regions, nhiều Mapped (file/IPC)
- TestTarget: ~50 regions, có 1 Private 4KB chứa int của ta

### 6.5 So sánh với Cheat Engine
[Bảng so sánh feature: scan time, UI complexity, code base size]

---

## CHƯƠNG 7 — KẾT LUẬN & HƯỚNG PHÁT TRIỂN

### 7.1 Kết luận
- Đã build được tool CheatVN với core features Cheat Engine
- Speedup multi-thread + SIMD đạt 30-43x
- Kiến trúc 2-layer dễ maintain + extend
- UI dark theme đẹp, không phụ thuộc thư viện ngoài

### 7.2 Hạn chế
- Pointer scan chỉ depth 1 (CE có 4-5)
- Chưa support String/Byte array scan
- Chưa có code injection / disassembler

### 7.3 Hướng phát triển
- **Pointer scan depth 4+**: tree search với offset range
- **Disassembler**: integrate Capstone library
- **Code injection / NOP**: patch instructions
- **String scan**: support UTF-8/16/ASCII patterns
- **Plugin system**: cho phép user viết Lua script (như CE)
- **Network**: scan process từ máy remote qua RDP

---

## TÀI LIỆU THAM KHẢO

1. Microsoft Docs — [Win32 Process and Thread Functions](https://learn.microsoft.com/en-us/windows/win32/procthread/process-and-thread-functions)
2. Microsoft Docs — [Memory Management Functions](https://learn.microsoft.com/en-us/windows/win32/memory/memory-management-functions)
3. Cheat Engine — https://github.com/cheat-engine/cheat-engine
4. Intel® Intrinsics Guide — https://www.intel.com/content/www/us/en/docs/intrinsics-guide/
5. Microsoft Docs — [C++/CLI Reference](https://learn.microsoft.com/en-us/cpp/dotnet/cpp-cli-language-reference)
6. .NET Framework — [Windows Forms](https://learn.microsoft.com/en-us/dotnet/desktop/winforms/)
7. James Forshaw — *Windows Security Internals*, No Starch Press 2024
8. Pavel Yosifovich — *Windows Internals 7th Edition*, Microsoft Press

---

## PHỤ LỤC

### A. Source code highlights
[Copy code chính như `firstScan` multi-thread + SIMD, `findPointersTo`, `DarkTheme::Apply`, ...]

### B. Screenshots
- Process List dialog
- Main scanner UI
- Hex Viewer
- Memory Heatmap
- Benchmark result
- Pointer Scanner result

### C. Build instructions
[Copy từ README]

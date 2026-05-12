# 01 — Tổng quan dự án CheatVN

> Tài liệu này giải thích **mục tiêu** và **kiến trúc** của CheatVN. Đọc trước khi xem 2 file kia.

## 1. CheatVN làm gì?

CheatVN là phần mềm có khả năng:
1. **Liệt kê** mọi process (chương trình) đang chạy trên máy tính
2. **Mở** một process mục tiêu (vd: notepad.exe, game.exe)
3. **Quét bộ nhớ** của process đó để tìm giá trị (vd: tìm địa chỉ chứa số máu của nhân vật trong game)
4. **Sửa** giá trị đó (vd: đổi máu từ 50 thành 9999)
5. **Đóng băng** giá trị (auto-ghi liên tục để giá trị không đổi)

Đây chính là cốt lõi của **Cheat Engine**. Trong báo cáo, ta gọi nó là *"công cụ phân tích và can thiệp bộ nhớ tiến trình"* — nghe học thuật hơn.

## 2. Tại sao đề tài này có "độ sâu"?

| Khía cạnh | Ta đụng tới |
|-----------|-------------|
| **Hệ điều hành** | Process, virtual memory, access token, privilege, page protection |
| **Thuật toán** | Quét hàng tỷ bytes nhanh: multi-threading, SIMD, lock-free |
| **Kiến trúc phần mềm** | Tách native/managed, bridge, marshalling |
| **Win32 API** | OpenProcess, ReadProcessMemory, VirtualQueryEx, IsWow64Process,... |
| **GUI design** | Dark theme, data binding, async refresh |

Thầy/cô chấm điểm sẽ ấn tượng vì bạn không chỉ "code app" mà **hiểu hệ điều hành** ở cấp độ kernel-API.

## 3. Kiến trúc 2 lớp

```
┌─────────────────────────────────────────────────────┐
│  LỚP GUI  (project "GUI")                           │
│  ───────────────────────────────────────────────    │
│  Ngôn ngữ: C++/CLI                                  │
│  Vai trò: cửa sổ, button, text box (Windows Forms)  │
│  Đặc tính: Managed code — chạy trên .NET runtime   │
└─────────────────────────────────────────────────────┘
                        ⇅
                  (cây cầu C++/CLI)
                        ⇅
┌─────────────────────────────────────────────────────┐
│  LỚP CORE  (project "Core")                         │
│  ───────────────────────────────────────────────    │
│  Ngôn ngữ: C++ thuần                                │
│  Vai trò: gọi Win32 API, quét memory, thuật toán    │
│  Đặc tính: Native code — chạy thẳng trên CPU       │
└─────────────────────────────────────────────────────┘
```

### Tại sao phải tách 2 lớp?

**Vấn đề**: code GUI muốn dễ viết (Windows Forms với drag-drop). Nhưng code GUI là **managed** — chạy chậm hơn native vì có Garbage Collector và overhead .NET. Mà việc quét memory cần **TỐC ĐỘ TỐI ĐA** (hàng tỷ phép so sánh).

**Giải pháp**: tách làm 2.
- GUI viết bằng C++/CLI (managed) → có designer, dễ làm UI đẹp
- Core viết bằng C++ thuần (native) → tốc độ tối đa, không overhead

→ Quy tắc vàng: **mọi thứ liên quan đến memory scan PHẢI nằm trong Core**.

### Ví dụ cụ thể

Khi user bấm nút "First Scan":
```
[User click button] (GUI)
       ↓
[Event handler trong GUI] (1 dòng code C++/CLI)
       ↓
[Gọi Bridge.FirstScan(value)]  ← vượt biên managed → native
       ↓
[Core::Scanner::firstScan(value)]  ← chạy native, full speed
       ↓
  (scan 4GB memory với 8 threads + SIMD trong 0.3 giây)
       ↓
[Return danh sách kết quả] ← vượt biên native → managed
       ↓
[GUI hiện kết quả lên DataGridView]
```

→ Chỉ có **2 lần vượt biên** quanh phần "scan 4GB". Nếu mỗi byte vượt biên thì sẽ chậm 100×.

## 4. Trạng thái hiện tại (đã xong)

✅ **Setup project**:
- Solution Visual Studio: `CheatVN.sln`
- Project Core (native static library, x64)
- Project GUI (C++/CLI WinForms App, x64, .NET 4.8)
- GUI link tới Core qua "Project Reference"

✅ **Core đầu tiên**:
- `ProcessInfo` (struct chứa info 1 process)
- `ProcessManager` (5 method: enum process, mở handle, bật privilege,...)

✅ **GUI đầu tiên**:
- `MainForm.h` (cửa sổ chính, hiện tại trắng)
- `Program.cpp` (`main()` chạy MainForm)

✅ **Mockup HTML**:
- `mockups/01_process_list.html` (preview dialog chọn process + bảng đặt tên control)

✅ **Git + GitHub**: code đã push lên https://github.com/Vanhung92qn/CheatVN

## 5. Lộ trình các phase

| Phase | Nội dung | Hiện trạng |
|-------|----------|------------|
| 1a | Setup VS solution | ✅ Done |
| 1b | Mockup ProcessListForm | ✅ Done |
| 1c | Core ProcessManager | ✅ Done |
| **1d** | **Bridge + ProcessListForm UI** | **🔄 Đang làm** |
| 2 | Scanner (FirstScan, NextScan, multi-thread, SIMD) | ⏳ Tuần 3-4 |
| 3 | Edit value, freeze, save session | ⏳ Tuần 5 |
| 4 | Hex viewer, Pointer scanner, Heatmap, Benchmark | ⏳ Tuần 6-7 |
| 5 | Polish, dark theme, viết báo cáo | ⏳ Tuần 8 |

## 6. Đọc tiếp

- 📘 **[02 — Kiến thức nền](02_KIEN_THUC_NEN.md)**: Visual Studio, Win32, C++/CLI là gì
- 📘 **[03 — Giải thích code](03_GIAI_THICH_CODE.md)**: walk-through từng file đã viết

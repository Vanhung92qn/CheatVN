#pragma once

// =====================================================================
//  DarkTheme.h
//
//  Helper static class — áp dụng dark theme cho mọi form/control.
//
//  CÁCH DÙNG:
//  ─────────
//  Trong constructor của form, sau InitializeComponent():
//      MainForm(void) {
//          InitializeComponent();
//          GUI::DarkTheme::Apply(this);   ← Một dòng duy nhất
//      }
//
//  Apply() sẽ:
//    1. Set BackColor + ForeColor của form
//    2. Đệ quy vào TẤT CẢ control con
//    3. Mỗi loại control có style riêng (Button khác TextBox khác DGV)
//
//  Khi cần màu cụ thể trong code:
//      this->BackColor = GUI::DarkTheme::WindowSurface;
//      pen = gcnew Pen(GUI::DarkTheme::Accent);
//
//  Bảng màu lấy từ GitHub Dark theme — đẹp, contrast tốt, đã được test.
// =====================================================================

namespace GUI {

    using namespace System::Drawing;
    using namespace System::Windows::Forms;

    public ref class DarkTheme abstract sealed {
    public:
        // ─── Bảng màu (static property dạng read-only getter) ────────
        // Dùng `static property` thay vì `static const Color` vì Color
        // là struct managed, không phải primitive — không thể literal.

        // Background ngoài cùng (cho app, viewport rộng nhất)
        static property Color AppBackground {
            Color get() { return Color::FromArgb(10, 10, 15); }
        }
        // Bề mặt form/cửa sổ chính
        static property Color WindowSurface {
            Color get() { return Color::FromArgb(13, 17, 23); }
        }
        // Panel/toolbar (1 lớp nổi hơn WindowSurface)
        static property Color PanelSurface {
            Color get() { return Color::FromArgb(22, 27, 34); }
        }
        // Trạng thái hover của button/menu item
        static property Color Hover {
            Color get() { return Color::FromArgb(33, 38, 45); }
        }
        // Border cho mọi thứ
        static property Color Border {
            Color get() { return Color::FromArgb(48, 54, 61); }
        }
        // Text chính (label, button text, body)
        static property Color TextPrimary {
            Color get() { return Color::FromArgb(201, 209, 217); }
        }
        // Text phụ (caption, hint)
        static property Color TextSecondary {
            Color get() { return Color::FromArgb(139, 148, 158); }
        }
        // Text mờ (status, watermark)
        static property Color TextMuted {
            Color get() { return Color::FromArgb(110, 118, 129); }
        }
        // Màu nhấn chính - xanh dương GitHub link
        static property Color Accent {
            Color get() { return Color::FromArgb(88, 166, 255); }
        }
        // Thành công (kết nối OK, scan xong)
        static property Color Success {
            Color get() { return Color::FromArgb(63, 185, 80); }
        }
        // Nguy hiểm (mất kết nối, lỗi)
        static property Color Danger {
            Color get() { return Color::FromArgb(248, 81, 73); }
        }
        // Giá trị memory (tím nhạt, hiển thị giá trị trong scan results)
        static property Color ValueColor {
            Color get() { return Color::FromArgb(210, 168, 255); }
        }
        // Địa chỉ memory (xanh nhạt, hiển thị 0x... trong results)
        static property Color AddressColor {
            Color get() { return Color::FromArgb(121, 192, 255); }
        }
        // Giá trị vừa thay đổi (cam, highlight ngắn 1-2s)
        static property Color ChangedColor {
            Color get() { return Color::FromArgb(247, 129, 102); }
        }

        // ─── Method công khai ────────────────────────────────────────

        // Áp dụng dark theme cho 1 control và toàn bộ control con.
        // Gọi 1 lần trong constructor sau InitializeComponent().
        static void Apply(Control^ control);
    };

} // namespace GUI

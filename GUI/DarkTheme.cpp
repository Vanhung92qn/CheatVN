// =====================================================================
//  DarkTheme.cpp
//
//  Implementation của DarkTheme::Apply().
//
//  Chiến lược:
//    1. Apply cho control hiện tại theo TYPE (Form / Button / TextBox /...)
//    2. Đệ quy vào tất cả control con (control->Controls)
//
//  Tại sao phải xử lý theo TYPE?
//  ─────────────────────────────
//  Mỗi loại WinForms control có property riêng để styling:
//    - Button:    FlatStyle, FlatAppearance.BorderColor
//    - TextBox:   BorderStyle (FixedSingle hoặc None)
//    - DataGrid:  rất nhiều property (xem ApplyDataGridView)
//    - ComboBox:  FlatStyle để dropdown ít glitch
//  Set BackColor chung cho mọi control SẼ KHÔNG đẹp — phải custom từng loại.
//
//  Tại sao đệ quy?
//  ──────────────
//  WinForms có hierarchy: Form chứa Panel, Panel chứa Button. Default
//  BackColor không tự cascade. Phải walk tree và set từng cái.
// =====================================================================

// ─── Native include cho DWM (Desktop Window Manager) API ────────────
// Đặt TRƯỚC #include "DarkTheme.h" vì các header này có macro ảnh hưởng
// đến phần managed. Pattern chuẩn cho file C++/CLI: native trước, managed sau.
#include <Windows.h>
#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")  // Link dwmapi.lib để có DwmSetWindowAttribute

#include "DarkTheme.h"

using namespace System;
using namespace System::Drawing;
using namespace System::Windows::Forms;

namespace GUI {

    // Forward-declare các helper riêng cho mỗi loại control.
    // Đặt trong anonymous namespace (private cho file này).
    namespace {

        // ─── Áp dụng dark title bar cho cửa sổ Windows 10/11 ─────
        //
        // Title bar (thanh trên cùng) KHÔNG do WinForms vẽ — do Desktop
        // Window Manager (DWM) của Windows vẽ. Set BackColor của Form
        // không ảnh hưởng được nó.
        //
        // Win10 build 1809+ có API `DwmSetWindowAttribute` với attribute
        // DWMWA_USE_IMMERSIVE_DARK_MODE để bảo DWM dùng dark title bar.
        //
        // Pitfall: giá trị attribute đổi qua các bản Windows:
        //   - Win10 1809–1909: attribute = 19
        //   - Win10 20H1+ và Win11: attribute = 20
        // Strategy: thử 20 trước, fail thì fallback 19.
        //
        // Trên Win7/8 → DwmSetWindowAttribute trả về lỗi → no-op (an toàn).
        void applyDarkTitleBar(HWND hwnd) {
            BOOL useDark = TRUE;

            // Thử với value 20 (Win11 / Win10 mới)
            HRESULT hr = DwmSetWindowAttribute(
                hwnd,
                20,                  // DWMWA_USE_IMMERSIVE_DARK_MODE
                &useDark,
                sizeof(useDark)
            );

            // Nếu fail (Win10 cũ), thử lại với value 19
            if (FAILED(hr)) {
                DwmSetWindowAttribute(
                    hwnd,
                    19,              // DWMWA_USE_IMMERSIVE_DARK_MODE_BEFORE_20H1
                    &useDark,
                    sizeof(useDark)
                );
            }
        }


        // Style cho DataGridView — phức tạp nhất, tách hàm riêng
        void styleDataGridView(DataGridView^ dgv) {
            // BackgroundColor (khác với BackColor!) — màu vùng ngoài cells,
            // hiện ra khi rows ít hơn chiều cao DGV.
            dgv->BackgroundColor = DarkTheme::WindowSurface;
            dgv->GridColor = DarkTheme::Border;
            dgv->BorderStyle = BorderStyle::None;

            // CRITICAL: phải tắt VisualStyles thì màu header mới được áp dụng.
            // Không tắt → header sẽ giữ style native (xanh nhạt).
            dgv->EnableHeadersVisualStyles = false;

            // Style cho cell thường (vùng data)
            auto cellStyle = gcnew DataGridViewCellStyle();
            cellStyle->BackColor = DarkTheme::WindowSurface;
            cellStyle->ForeColor = DarkTheme::TextPrimary;
            cellStyle->SelectionBackColor = DarkTheme::Accent;
            cellStyle->SelectionForeColor = Color::White;
            cellStyle->Padding = System::Windows::Forms::Padding(4, 2, 4, 2);
            dgv->DefaultCellStyle = cellStyle;

            // Style cho header column (dòng đầu, tên cột)
            auto headerStyle = gcnew DataGridViewCellStyle();
            headerStyle->BackColor = DarkTheme::PanelSurface;
            headerStyle->ForeColor = DarkTheme::TextSecondary;
            // Khi click vào header để sort, không đổi màu (giữ nguyên)
            headerStyle->SelectionBackColor = DarkTheme::PanelSurface;
            headerStyle->SelectionForeColor = DarkTheme::TextSecondary;
            headerStyle->Padding = System::Windows::Forms::Padding(6, 4, 6, 4);
            dgv->ColumnHeadersDefaultCellStyle = headerStyle;

            // Style cho alternating rows (mỗi dòng chẵn có màu khác nhẹ)
            // Tạo hiệu ứng zebra để dễ đọc dòng nào dòng nấy.
            auto altStyle = gcnew DataGridViewCellStyle();
            altStyle->BackColor = DarkTheme::PanelSurface;
            altStyle->ForeColor = DarkTheme::TextPrimary;
            altStyle->SelectionBackColor = DarkTheme::Accent;
            altStyle->SelectionForeColor = Color::White;
            dgv->AlternatingRowsDefaultCellStyle = altStyle;

            // Tắt row header (cột nhỏ bên trái có icon mũi tên) — chiếm chỗ
            dgv->RowHeadersVisible = false;
        }

        // Style cho Button — biến thành "flat button" với border tùy chỉnh
        void styleButton(Button^ btn) {
            btn->BackColor = DarkTheme::PanelSurface;
            btn->ForeColor = DarkTheme::TextPrimary;

            // FlatStyle::Flat = bỏ border 3D mặc định, cho phép tùy biến border
            btn->FlatStyle = FlatStyle::Flat;

            // FlatAppearance là 1 struct con của Button — chứa border + hover color
            btn->FlatAppearance->BorderColor = DarkTheme::Border;
            btn->FlatAppearance->BorderSize = 1;
            btn->FlatAppearance->MouseOverBackColor = DarkTheme::Hover;
            btn->FlatAppearance->MouseDownBackColor = DarkTheme::Border;

            // Cursor pointer khi hover → cảm giác clickable hơn
            btn->Cursor = Cursors::Hand;

            // Padding text nhẹ
            btn->Padding = System::Windows::Forms::Padding(4, 2, 4, 2);
        }

        // Style cho TextBox — viền 1px thay vì 3D
        void styleTextBox(TextBox^ tb) {
            tb->BackColor = DarkTheme::AppBackground;
            tb->ForeColor = DarkTheme::TextPrimary;
            // BorderStyle::FixedSingle = viền 1px (vẫn xám native — không đổi được màu).
            // Để màu border tùy chỉnh, phải wrap trong Panel + tự vẽ — Phase 5.
            tb->BorderStyle = BorderStyle::FixedSingle;
        }

        // Style cho CheckBox / RadioButton — chỉ ForeColor, giữ BackColor parent
        void styleCheckBox(CheckBox^ cb) {
            cb->ForeColor = DarkTheme::TextPrimary;
            cb->BackColor = Color::Transparent;  // Lấy màu của parent panel
            cb->FlatStyle = FlatStyle::Flat;
            cb->Cursor = Cursors::Hand;
        }

        void styleRadioButton(RadioButton^ rb) {
            rb->ForeColor = DarkTheme::TextPrimary;
            rb->BackColor = Color::Transparent;
            rb->FlatStyle = FlatStyle::Flat;
            rb->Cursor = Cursors::Hand;
        }

        // Style cho ComboBox — FlatStyle Flat giúp dropdown trông nhất quán
        void styleComboBox(ComboBox^ cmb) {
            cmb->BackColor = DarkTheme::AppBackground;
            cmb->ForeColor = DarkTheme::TextPrimary;
            cmb->FlatStyle = FlatStyle::Flat;
            // Lưu ý: dropdown list mở ra vẫn theo theme Windows native — khó tùy biến
            // hoàn toàn nếu không tự owner-draw. Chấp nhận được cho Phase 1.
        }

        // Style cho Label — trong suốt, lấy màu nền parent
        void styleLabel(Label^ lbl) {
            lbl->BackColor = Color::Transparent;
            // Giữ ForeColor mặc định = inherit từ form (TextPrimary).
            // Trừ khi user set khác trong designer (sẽ tôn trọng).
        }

        // Style cho GroupBox — frame chứa các control
        void styleGroupBox(GroupBox^ gb) {
            gb->ForeColor = DarkTheme::TextSecondary;
            gb->BackColor = Color::Transparent;
        }

        // Style cho ListBox
        void styleListBox(ListBox^ lb) {
            lb->BackColor = DarkTheme::WindowSurface;
            lb->ForeColor = DarkTheme::TextPrimary;
            lb->BorderStyle = BorderStyle::FixedSingle;
        }

        // Style cho Panel — chỉ BackColor (tạo "layer" cảm giác chiều sâu)
        // KHÔNG đệ quy ở đây — sẽ làm ở Apply() ngoài.
        void stylePanel(Panel^ p) {
            // Panel mặc định lấy màu form (WindowSurface). Nếu user muốn tạo
            // toolbar khác màu thì set trong designer (vd: PanelSurface).
            // Mình không override để tôn trọng thiết kế designer.
        }

        // Style cho StatusStrip — đáy form
        void styleStatusStrip(StatusStrip^ ss) {
            ss->BackColor = DarkTheme::PanelSurface;
            ss->ForeColor = DarkTheme::TextMuted;
        }

        // Style cho MenuStrip — đỉnh form (nếu có menu)
        void styleMenuStrip(MenuStrip^ ms) {
            ms->BackColor = DarkTheme::PanelSurface;
            ms->ForeColor = DarkTheme::TextPrimary;
        }

        // Style cho Form — entry point của theme
        void styleForm(Form^ form) {
            form->BackColor = DarkTheme::WindowSurface;
            form->ForeColor = DarkTheme::TextPrimary;
            // Font cascade tự động xuống children nếu chúng không override
            if (form->Font == nullptr || form->Font->Name == "Microsoft Sans Serif") {
                form->Font = gcnew Font("Segoe UI", 9.0f);
            }

            // ─── Áp dụng dark title bar ──────────────────────────
            // form->Handle là property — khi truy cập lần đầu, WinForms
            // sẽ TRIGGER tạo HWND (native window handle). Tới đây, form
            // đã có handle thật để truyền cho DWM API.
            //
            // IntPtr (managed) chuyển sang HWND (native void*) qua
            // ToPointer() — kiểu cast IntPtr → void* → HWND.
            applyDarkTitleBar((HWND)form->Handle.ToPointer());
        }

    } // anonymous namespace

    // =================================================================
    //  Apply — dispatcher chính
    // =================================================================
    void DarkTheme::Apply(Control^ control) {
        if (control == nullptr) return;

        // ─── Bước 1: dispatch theo TYPE ───────────────────────────
        // Dùng dynamic_cast để check type ở runtime. Tương đương C#:
        //     if (control is Form form) { ... }

        if (auto form = dynamic_cast<Form^>(control)) {
            styleForm(form);
        }
        else if (auto btn = dynamic_cast<Button^>(control)) {
            styleButton(btn);
        }
        else if (auto tb = dynamic_cast<TextBox^>(control)) {
            styleTextBox(tb);
        }
        else if (auto dgv = dynamic_cast<DataGridView^>(control)) {
            styleDataGridView(dgv);
        }
        else if (auto cb = dynamic_cast<CheckBox^>(control)) {
            styleCheckBox(cb);
        }
        else if (auto rb = dynamic_cast<RadioButton^>(control)) {
            styleRadioButton(rb);
        }
        else if (auto cmb = dynamic_cast<ComboBox^>(control)) {
            styleComboBox(cmb);
        }
        else if (auto lbl = dynamic_cast<Label^>(control)) {
            styleLabel(lbl);
        }
        else if (auto gb = dynamic_cast<GroupBox^>(control)) {
            styleGroupBox(gb);
        }
        else if (auto lb = dynamic_cast<ListBox^>(control)) {
            styleListBox(lb);
        }
        else if (auto ss = dynamic_cast<StatusStrip^>(control)) {
            styleStatusStrip(ss);
        }
        else if (auto ms = dynamic_cast<MenuStrip^>(control)) {
            styleMenuStrip(ms);
        }
        else if (auto p = dynamic_cast<Panel^>(control)) {
            stylePanel(p);
        }
        // Nếu type không match cái nào ở trên thì để default — không sao.

        // ─── Bước 2: đệ quy vào control con ────────────────────────
        // control->Controls trả về collection các child control.
        // for each (loop quản lý) chạy như foreach C#.
        for each (Control^ child in control->Controls) {
            Apply(child);
        }
    }

} // namespace GUI

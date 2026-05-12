#pragma once

#include "DarkTheme.h"
#include "ScannerBridge.h"

namespace GUI {

    using namespace System;
    using namespace System::ComponentModel;
    using namespace System::Collections::Generic;
    using namespace System::Windows::Forms;
    using namespace System::Drawing;
    using namespace System::Diagnostics;

    /// <summary>
    /// PointerScannerForm — tìm địa chỉ chứa pointer trỏ đến targetAddress.
    /// Use case: HP address của game đổi sau restart, nhưng pointer trỏ tới
    /// HP thường nằm trong static memory. Tìm "ai trỏ tới HP" → có địa chỉ
    /// tĩnh để lock value.
    ///
    /// Đây là phiên bản depth 1 đơn giản. Cheat Engine có depth 4-5 với
    /// offset range — không implement vì phức tạp.
    /// </summary>
    public ref class PointerScannerForm : public System::Windows::Forms::Form
    {
    public:
        PointerScannerForm(IntPtr hProcess, UInt64 initialTarget)
        {
            _hProcess = hProcess;
            InitializeComponent();
            DarkTheme::Apply(this);
            this->btnScan->BackColor = DarkTheme::Accent;
            this->btnScan->ForeColor = Color::White;
            this->btnScan->FlatStyle = System::Windows::Forms::FlatStyle::Flat;

            this->txtTarget->Text = String::Format(L"0x{0:X16}", initialTarget);
            this->btnScan->Click += gcnew EventHandler(this, &PointerScannerForm::OnScanClick);
            this->btnClose->Click += gcnew EventHandler(this, &PointerScannerForm::OnCloseClick);
        }

    protected:
        ~PointerScannerForm() { if (components) delete components; }

    private:
        IntPtr _hProcess;
        System::ComponentModel::IContainer^ components;
        System::Windows::Forms::Panel^ pnlTop;
        System::Windows::Forms::Label^ lblTarget;
        System::Windows::Forms::TextBox^ txtTarget;
        System::Windows::Forms::Button^ btnScan;
        System::Windows::Forms::Label^ lblHint;
        System::Windows::Forms::ListBox^ lstResults;
        System::Windows::Forms::Panel^ pnlBottom;
        System::Windows::Forms::Button^ btnClose;
        System::Windows::Forms::Label^ lblStatus;

#pragma region Windows Form Designer generated code
        void InitializeComponent(void)
        {
            this->components = gcnew System::ComponentModel::Container();
            this->pnlTop = gcnew System::Windows::Forms::Panel();
            this->lblTarget = gcnew System::Windows::Forms::Label();
            this->txtTarget = gcnew System::Windows::Forms::TextBox();
            this->btnScan = gcnew System::Windows::Forms::Button();
            this->lblHint = gcnew System::Windows::Forms::Label();
            this->lstResults = gcnew System::Windows::Forms::ListBox();
            this->pnlBottom = gcnew System::Windows::Forms::Panel();
            this->btnClose = gcnew System::Windows::Forms::Button();
            this->lblStatus = gcnew System::Windows::Forms::Label();

            this->pnlTop->SuspendLayout();
            this->pnlBottom->SuspendLayout();
            this->SuspendLayout();

            this->pnlTop->Controls->Add(this->lblHint);
            this->pnlTop->Controls->Add(this->btnScan);
            this->pnlTop->Controls->Add(this->txtTarget);
            this->pnlTop->Controls->Add(this->lblTarget);
            this->pnlTop->Dock = System::Windows::Forms::DockStyle::Top;
            this->pnlTop->Size = System::Drawing::Size(700, 90);

            this->lblTarget->AutoSize = true;
            this->lblTarget->Location = System::Drawing::Point(12, 14);
            this->lblTarget->Text = L"Địa chỉ đích (mục tiêu trỏ tới):";

            this->txtTarget->Location = System::Drawing::Point(12, 36);
            this->txtTarget->Size = System::Drawing::Size(280, 23);
            this->txtTarget->Font = gcnew System::Drawing::Font(L"Consolas", 10.0f);

            this->btnScan->Location = System::Drawing::Point(310, 33);
            this->btnScan->Size = System::Drawing::Size(150, 30);
            this->btnScan->Text = L"Quét pointer";

            this->lblHint->Location = System::Drawing::Point(12, 66);
            this->lblHint->Size = System::Drawing::Size(680, 18);
            this->lblHint->Text = L"Quét toàn bộ memory tìm uint64 = địa chỉ đích (depth 1, multi-thread)";
            this->lblHint->ForeColor = DarkTheme::TextMuted;

            this->lstResults->Dock = System::Windows::Forms::DockStyle::Fill;
            this->lstResults->Font = gcnew System::Drawing::Font(L"Consolas", 10.0f);
            this->lstResults->BackColor = DarkTheme::WindowSurface;
            this->lstResults->ForeColor = DarkTheme::TextPrimary;
            this->lstResults->BorderStyle = System::Windows::Forms::BorderStyle::FixedSingle;

            this->pnlBottom->Controls->Add(this->btnClose);
            this->pnlBottom->Controls->Add(this->lblStatus);
            this->pnlBottom->Dock = System::Windows::Forms::DockStyle::Bottom;
            this->pnlBottom->Size = System::Drawing::Size(700, 44);

            this->lblStatus->AutoSize = false;
            this->lblStatus->Location = System::Drawing::Point(12, 12);
            this->lblStatus->Size = System::Drawing::Size(500, 20);
            this->lblStatus->Text = L"";

            this->btnClose->Location = System::Drawing::Point(600, 8);
            this->btnClose->Size = System::Drawing::Size(90, 28);
            this->btnClose->Text = L"Đóng";
            this->btnClose->Anchor = static_cast<System::Windows::Forms::AnchorStyles>(
                System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Right);

            this->AutoScaleDimensions = System::Drawing::SizeF(7, 15);
            this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
            this->ClientSize = System::Drawing::Size(700, 500);
            this->Controls->Add(this->lstResults);
            this->Controls->Add(this->pnlTop);
            this->Controls->Add(this->pnlBottom);
            this->Name = L"PointerScannerForm";
            this->Text = L"Pointer Scanner — CheatVN";
            this->StartPosition = System::Windows::Forms::FormStartPosition::CenterParent;

            this->pnlTop->ResumeLayout(false);
            this->pnlTop->PerformLayout();
            this->pnlBottom->ResumeLayout(false);
            this->pnlBottom->PerformLayout();
            this->ResumeLayout(false);
        }
#pragma endregion

        void OnCloseClick(Object^ sender, EventArgs^ e) { this->Close(); }

        void OnScanClick(Object^ sender, EventArgs^ e) {
            // Parse target hex
            String^ text = this->txtTarget->Text->Trim();
            if (text->StartsWith(L"0x") || text->StartsWith(L"0X"))
                text = text->Substring(2);

            UInt64 target;
            try { target = Convert::ToUInt64(text, 16); }
            catch (Exception^) {
                MessageBox::Show(this, L"Địa chỉ không hợp lệ", L"Lỗi",
                    MessageBoxButtons::OK, MessageBoxIcon::Warning);
                return;
            }

            this->Cursor = Cursors::WaitCursor;
            this->btnScan->Enabled = false;
            this->lstResults->Items->Clear();
            this->lblStatus->Text = L"Đang quét pointer...";
            Application::DoEvents();

            auto sw = Stopwatch::StartNew();
            auto pointers = ScannerBridge::FindPointersTo(_hProcess, target);
            sw->Stop();

            // Limit display 1000 first results để tránh lag
            int displayCount = Math::Min(pointers->Count, 1000);
            for (int i = 0; i < displayCount; ++i) {
                this->lstResults->Items->Add(
                    String::Format(L"0x{0:X16}  →  0x{1:X16}", pointers[i], target));
            }

            this->lblStatus->Text = String::Format(
                L"Tìm thấy {0} pointer trỏ đến 0x{1:X16}  ({2:F2}s){3}",
                pointers->Count, target, sw->Elapsed.TotalSeconds,
                pointers->Count > 1000 ? L"  — chỉ hiển thị 1000 đầu" : L"");
            this->lblStatus->ForeColor = pointers->Count > 0 ?
                DarkTheme::Success : DarkTheme::TextMuted;

            this->Cursor = Cursors::Default;
            this->btnScan->Enabled = true;
        }
    };

} // namespace GUI

#pragma once

#include "DarkTheme.h"
#include "ScannerBridge.h"

namespace GUI {

    using namespace System;
    using namespace System::ComponentModel;
    using namespace System::Collections::Generic;
    using namespace System::Windows::Forms;
    using namespace System::Drawing;
    using namespace System::Text;

    /// <summary>
    /// HexViewerForm — cửa sổ xem raw bytes của memory process target.
    ///
    /// Format hiển thị (như HxD / Cheat Engine Memory Viewer):
    ///   ADDRESS         | HEX BYTES (16)                              | ASCII
    ///   0x00007FF600000 | 48 65 6C 6C 6F 20 57 6F 72 6C 64 21 00 00 00 00 | Hello World!....
    ///   0x00007FF600010 | ...                                              | ...
    ///
    /// Auto-refresh mỗi 500ms để thấy data thay đổi live.
    /// </summary>
    public ref class HexViewerForm : public System::Windows::Forms::Form
    {
    public:
        HexViewerForm(IntPtr hProcess, UInt64 startAddress)
        {
            _hProcess = hProcess;
            _baseAddress = startAddress;
            // Hiển thị 256 bytes (= 16 dòng × 16 bytes/dòng)
            _byteCount = 256;

            InitializeComponent();
            DarkTheme::Apply(this);

            // RichTextBox dùng monospace + dark
            this->rtbHex->BackColor = DarkTheme::WindowSurface;
            this->rtbHex->ForeColor = DarkTheme::TextPrimary;
            this->rtbHex->Font = gcnew System::Drawing::Font(L"Consolas", 10.0f);

            this->btnRefresh->Click +=
                gcnew EventHandler(this, &HexViewerForm::OnRefreshClick);
            this->btnGoto->Click +=
                gcnew EventHandler(this, &HexViewerForm::OnGotoClick);
            this->btnClose->Click +=
                gcnew EventHandler(this, &HexViewerForm::OnCloseClick);
            this->Load += gcnew EventHandler(this, &HexViewerForm::OnFormLoad);
            this->timerRefresh->Tick +=
                gcnew EventHandler(this, &HexViewerForm::OnTimerTick);
        }

    protected:
        ~HexViewerForm() { if (components) delete components; }

    private:
        IntPtr _hProcess;
        UInt64 _baseAddress;
        int _byteCount;

        System::ComponentModel::IContainer^ components;
        System::Windows::Forms::Panel^ pnlTop;
        System::Windows::Forms::Label^ lblAddressLabel;
        System::Windows::Forms::TextBox^ txtAddress;
        System::Windows::Forms::Button^ btnGoto;
        System::Windows::Forms::Button^ btnRefresh;
        System::Windows::Forms::RichTextBox^ rtbHex;
        System::Windows::Forms::Panel^ pnlBottom;
        System::Windows::Forms::Button^ btnClose;
        System::Windows::Forms::Label^ lblStatus;
        System::Windows::Forms::Timer^ timerRefresh;

#pragma region Windows Form Designer generated code
        void InitializeComponent(void)
        {
            this->components = gcnew System::ComponentModel::Container();
            this->pnlTop = gcnew System::Windows::Forms::Panel();
            this->lblAddressLabel = gcnew System::Windows::Forms::Label();
            this->txtAddress = gcnew System::Windows::Forms::TextBox();
            this->btnGoto = gcnew System::Windows::Forms::Button();
            this->btnRefresh = gcnew System::Windows::Forms::Button();
            this->rtbHex = gcnew System::Windows::Forms::RichTextBox();
            this->pnlBottom = gcnew System::Windows::Forms::Panel();
            this->btnClose = gcnew System::Windows::Forms::Button();
            this->lblStatus = gcnew System::Windows::Forms::Label();
            this->timerRefresh = gcnew System::Windows::Forms::Timer(this->components);

            this->pnlTop->SuspendLayout();
            this->pnlBottom->SuspendLayout();
            this->SuspendLayout();

            // pnlTop
            this->pnlTop->Controls->Add(this->btnRefresh);
            this->pnlTop->Controls->Add(this->btnGoto);
            this->pnlTop->Controls->Add(this->txtAddress);
            this->pnlTop->Controls->Add(this->lblAddressLabel);
            this->pnlTop->Dock = System::Windows::Forms::DockStyle::Top;
            this->pnlTop->Location = System::Drawing::Point(0, 0);
            this->pnlTop->Name = L"pnlTop";
            this->pnlTop->Size = System::Drawing::Size(800, 50);

            // lblAddressLabel
            this->lblAddressLabel->AutoSize = true;
            this->lblAddressLabel->Location = System::Drawing::Point(12, 17);
            this->lblAddressLabel->Name = L"lblAddressLabel";
            this->lblAddressLabel->Text = L"Địa chỉ:";

            // txtAddress
            this->txtAddress->Location = System::Drawing::Point(70, 14);
            this->txtAddress->Name = L"txtAddress";
            this->txtAddress->Size = System::Drawing::Size(220, 23);
            this->txtAddress->Font = gcnew System::Drawing::Font(L"Consolas", 9.0f);

            // btnGoto
            this->btnGoto->Location = System::Drawing::Point(300, 12);
            this->btnGoto->Name = L"btnGoto";
            this->btnGoto->Size = System::Drawing::Size(90, 26);
            this->btnGoto->Text = L"Đi tới";

            // btnRefresh
            this->btnRefresh->Location = System::Drawing::Point(400, 12);
            this->btnRefresh->Name = L"btnRefresh";
            this->btnRefresh->Size = System::Drawing::Size(90, 26);
            this->btnRefresh->Text = L"Refresh";

            // rtbHex
            this->rtbHex->Dock = System::Windows::Forms::DockStyle::Fill;
            this->rtbHex->Name = L"rtbHex";
            this->rtbHex->ReadOnly = true;
            this->rtbHex->WordWrap = false;
            this->rtbHex->ScrollBars = System::Windows::Forms::RichTextBoxScrollBars::Both;

            // pnlBottom
            this->pnlBottom->Controls->Add(this->btnClose);
            this->pnlBottom->Controls->Add(this->lblStatus);
            this->pnlBottom->Dock = System::Windows::Forms::DockStyle::Bottom;
            this->pnlBottom->Location = System::Drawing::Point(0, 510);
            this->pnlBottom->Name = L"pnlBottom";
            this->pnlBottom->Size = System::Drawing::Size(800, 40);

            // lblStatus
            this->lblStatus->AutoSize = false;
            this->lblStatus->Location = System::Drawing::Point(12, 12);
            this->lblStatus->Size = System::Drawing::Size(580, 20);
            this->lblStatus->Name = L"lblStatus";
            this->lblStatus->Text = L"";
            this->lblStatus->TextAlign = System::Drawing::ContentAlignment::MiddleLeft;

            // btnClose
            this->btnClose->Location = System::Drawing::Point(700, 6);
            this->btnClose->Size = System::Drawing::Size(90, 28);
            this->btnClose->Name = L"btnClose";
            this->btnClose->Text = L"Đóng";
            this->btnClose->Anchor = static_cast<System::Windows::Forms::AnchorStyles>(
                System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Right);

            // timerRefresh
            this->timerRefresh->Interval = 500;
            this->timerRefresh->Enabled = true;

            // Form
            this->AutoScaleDimensions = System::Drawing::SizeF(7, 15);
            this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
            this->ClientSize = System::Drawing::Size(800, 550);
            this->Controls->Add(this->rtbHex);
            this->Controls->Add(this->pnlTop);
            this->Controls->Add(this->pnlBottom);
            this->Name = L"HexViewerForm";
            this->Text = L"Hex Viewer — CheatVN";
            this->StartPosition = System::Windows::Forms::FormStartPosition::CenterParent;
            this->MinimizeBox = false;
            this->MaximizeBox = true;

            this->pnlTop->ResumeLayout(false);
            this->pnlTop->PerformLayout();
            this->pnlBottom->ResumeLayout(false);
            this->pnlBottom->PerformLayout();
            this->ResumeLayout(false);
        }
#pragma endregion

        void OnFormLoad(Object^ sender, EventArgs^ e) {
            this->txtAddress->Text = String::Format(L"0x{0:X16}", _baseAddress);
            RefreshHex();
        }

        void OnTimerTick(Object^ sender, EventArgs^ e) {
            RefreshHex();
        }

        void OnRefreshClick(Object^ sender, EventArgs^ e) {
            RefreshHex();
        }

        void OnCloseClick(Object^ sender, EventArgs^ e) {
            this->Close();
        }

        void OnGotoClick(Object^ sender, EventArgs^ e) {
            // Parse địa chỉ từ textbox (chấp nhận "0x..." hoặc thuần hex)
            String^ text = this->txtAddress->Text->Trim();
            if (text->StartsWith(L"0x") || text->StartsWith(L"0X")) {
                text = text->Substring(2);
            }
            try {
                _baseAddress = Convert::ToUInt64(text, 16);
                RefreshHex();
            }
            catch (Exception^) {
                MessageBox::Show(this,
                    L"Địa chỉ không hợp lệ. Hãy nhập dạng 0x... hoặc hex thuần.",
                    L"Lỗi", MessageBoxButtons::OK, MessageBoxIcon::Warning);
            }
        }

        // =================================================================
        //  RefreshHex — đọc memory + format hiển thị
        // =================================================================
        // Đọc 256 bytes (16 dòng × 16 bytes), format theo style HxD:
        //   ADDRESS  | HEX (mỗi byte 2 chars + space) | ASCII (printable)
        void RefreshHex() {
            // Save scroll position để không nhảy về top sau refresh
            int caretPos = this->rtbHex->SelectionStart;

            auto bytes = ScannerBridge::ReadBytes(_hProcess, _baseAddress, _byteCount);
            if (bytes->Length == 0) {
                this->rtbHex->Text = L"(Không đọc được memory tại địa chỉ này)";
                this->lblStatus->Text = String::Format(
                    L"Read fail tại 0x{0:X16}", _baseAddress);
                this->lblStatus->ForeColor = DarkTheme::Danger;
                return;
            }

            auto sb = gcnew StringBuilder();
            sb->AppendLine(L"OFFSET (h) | 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F | DECODED");
            sb->AppendLine(L"-----------+-------------------------------------------------+----------------");

            // Iterate 16 bytes mỗi dòng
            for (int row = 0; row < bytes->Length; row += 16) {
                // Cột 1: address của row
                sb->AppendFormat(L"0x{0:X8} | ", _baseAddress + (UInt64)row);

                // Cột 2: 16 bytes dạng hex
                StringBuilder^ ascii = gcnew StringBuilder();
                for (int col = 0; col < 16; col++) {
                    int idx = row + col;
                    if (idx < bytes->Length) {
                        sb->AppendFormat(L"{0:X2} ", bytes[idx]);
                        // Cột 3: ASCII (printable ASCII 32-126)
                        Byte b = bytes[idx];
                        if (b >= 32 && b < 127) {
                            ascii->Append((wchar_t)b);
                        } else {
                            ascii->Append(L'.');
                        }
                    } else {
                        sb->Append(L"   ");
                        ascii->Append(L' ');
                    }
                }

                sb->Append(L"| ");
                sb->AppendLine(ascii->ToString());
            }

            this->rtbHex->Text = sb->ToString();

            // Restore caret
            try {
                if (caretPos < this->rtbHex->TextLength) {
                    this->rtbHex->SelectionStart = caretPos;
                }
            } catch (Exception^) { /* ignore */ }

            this->lblStatus->Text = String::Format(
                L"Đọc {0} bytes từ 0x{1:X16} (auto-refresh 500ms)",
                bytes->Length, _baseAddress);
            this->lblStatus->ForeColor = DarkTheme::TextSecondary;
        }
    };

} // namespace GUI

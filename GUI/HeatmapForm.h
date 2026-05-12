#pragma once

#include "DarkTheme.h"
#include "ScannerBridge.h"

namespace GUI {

    using namespace System;
    using namespace System::ComponentModel;
    using namespace System::Collections::Generic;
    using namespace System::Windows::Forms;
    using namespace System::Drawing;
    using namespace System::Drawing::Drawing2D;

    /// <summary>
    /// HeatmapForm — visualize tất cả memory regions của process target.
    /// Top: custom-paint Panel hiển thị "ribbon" các region màu theo type
    /// Bottom: DataGridView liệt kê chi tiết từng region
    /// </summary>
    public ref class HeatmapForm : public System::Windows::Forms::Form
    {
    public:
        HeatmapForm(IntPtr hProcess, String^ processName)
        {
            _hProcess = hProcess;
            _processName = processName;
            InitializeComponent();
            DarkTheme::Apply(this);

            this->pnlCanvas->BackColor = DarkTheme::AppBackground;
            this->pnlCanvas->Paint +=
                gcnew PaintEventHandler(this, &HeatmapForm::OnCanvasPaint);

            this->btnRefresh->Click +=
                gcnew EventHandler(this, &HeatmapForm::OnRefreshClick);
            this->btnClose->Click +=
                gcnew EventHandler(this, &HeatmapForm::OnCloseClick);
            this->Load += gcnew EventHandler(this, &HeatmapForm::OnFormLoad);
        }

    protected:
        ~HeatmapForm() { if (components) delete components; }

    private:
        IntPtr _hProcess;
        String^ _processName;
        List<ManagedMemoryRegion^>^ _regions;

        System::ComponentModel::IContainer^ components;
        System::Windows::Forms::Panel^ pnlTop;
        System::Windows::Forms::Label^ lblTitle;
        System::Windows::Forms::Label^ lblLegend;
        System::Windows::Forms::Button^ btnRefresh;
        System::Windows::Forms::Panel^ pnlCanvas;
        System::Windows::Forms::DataGridView^ dgvRegions;
        System::Windows::Forms::DataGridViewTextBoxColumn^ colAddr;
        System::Windows::Forms::DataGridViewTextBoxColumn^ colSize;
        System::Windows::Forms::DataGridViewTextBoxColumn^ colType;
        System::Windows::Forms::DataGridViewTextBoxColumn^ colProt;
        System::Windows::Forms::Panel^ pnlBottom;
        System::Windows::Forms::Button^ btnClose;
        System::Windows::Forms::Label^ lblStatus;

#pragma region Windows Form Designer generated code
        void InitializeComponent(void)
        {
            this->components = gcnew System::ComponentModel::Container();
            this->pnlTop = gcnew System::Windows::Forms::Panel();
            this->lblTitle = gcnew System::Windows::Forms::Label();
            this->lblLegend = gcnew System::Windows::Forms::Label();
            this->btnRefresh = gcnew System::Windows::Forms::Button();
            this->pnlCanvas = gcnew System::Windows::Forms::Panel();
            this->dgvRegions = gcnew System::Windows::Forms::DataGridView();
            this->colAddr = gcnew System::Windows::Forms::DataGridViewTextBoxColumn();
            this->colSize = gcnew System::Windows::Forms::DataGridViewTextBoxColumn();
            this->colType = gcnew System::Windows::Forms::DataGridViewTextBoxColumn();
            this->colProt = gcnew System::Windows::Forms::DataGridViewTextBoxColumn();
            this->pnlBottom = gcnew System::Windows::Forms::Panel();
            this->btnClose = gcnew System::Windows::Forms::Button();
            this->lblStatus = gcnew System::Windows::Forms::Label();

            this->pnlTop->SuspendLayout();
            (cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->dgvRegions))->BeginInit();
            this->pnlBottom->SuspendLayout();
            this->SuspendLayout();

            // pnlTop
            this->pnlTop->Controls->Add(this->btnRefresh);
            this->pnlTop->Controls->Add(this->lblLegend);
            this->pnlTop->Controls->Add(this->lblTitle);
            this->pnlTop->Dock = System::Windows::Forms::DockStyle::Top;
            this->pnlTop->Size = System::Drawing::Size(1100, 60);

            this->lblTitle->AutoSize = true;
            this->lblTitle->Location = System::Drawing::Point(12, 8);
            this->lblTitle->Text = L"Memory Heatmap";
            this->lblTitle->Font = gcnew System::Drawing::Font(L"Segoe UI", 11.0f, System::Drawing::FontStyle::Bold);

            this->lblLegend->AutoSize = true;
            this->lblLegend->Location = System::Drawing::Point(12, 34);
            this->lblLegend->Text = L"■ Image (exe/dll)    ■ Private (heap/stack)    ■ Mapped (file)    [W] = writable, [R] = read-only";

            this->btnRefresh->Location = System::Drawing::Point(960, 16);
            this->btnRefresh->Size = System::Drawing::Size(130, 30);
            this->btnRefresh->Text = L"Refresh";
            this->btnRefresh->Anchor = static_cast<System::Windows::Forms::AnchorStyles>(
                System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Right);

            // pnlCanvas — custom paint visualization
            this->pnlCanvas->Dock = System::Windows::Forms::DockStyle::Top;
            this->pnlCanvas->Location = System::Drawing::Point(0, 60);
            this->pnlCanvas->Size = System::Drawing::Size(1100, 100);

            // dgvRegions
            this->dgvRegions->AllowUserToAddRows = false;
            this->dgvRegions->AllowUserToDeleteRows = false;
            this->dgvRegions->AllowUserToResizeRows = false;
            this->dgvRegions->AutoGenerateColumns = false;
            this->dgvRegions->ColumnHeadersHeight = 28;
            this->dgvRegions->Columns->AddRange(
                gcnew cli::array<System::Windows::Forms::DataGridViewColumn^>(4) {
                    this->colAddr, this->colSize, this->colType, this->colProt
                });
            this->dgvRegions->Dock = System::Windows::Forms::DockStyle::Fill;
            this->dgvRegions->ReadOnly = true;
            this->dgvRegions->RowHeadersVisible = false;
            this->dgvRegions->SelectionMode = System::Windows::Forms::DataGridViewSelectionMode::FullRowSelect;

            this->colAddr->DataPropertyName = L"AddressDisplay";
            this->colAddr->HeaderText = L"Địa chỉ bắt đầu";
            this->colAddr->Width = 220;
            this->colAddr->DefaultCellStyle->Font = gcnew System::Drawing::Font(L"Consolas", 9.0f);

            this->colSize->DataPropertyName = L"SizeDisplay";
            this->colSize->HeaderText = L"Kích thước";
            this->colSize->Width = 130;

            this->colType->DataPropertyName = L"TypeDisplay";
            this->colType->HeaderText = L"Loại";
            this->colType->Width = 200;

            this->colProt->DataPropertyName = L"ProtectionDisplay";
            this->colProt->HeaderText = L"Quyền";
            this->colProt->Width = 100;

            // pnlBottom
            this->pnlBottom->Controls->Add(this->btnClose);
            this->pnlBottom->Controls->Add(this->lblStatus);
            this->pnlBottom->Dock = System::Windows::Forms::DockStyle::Bottom;
            this->pnlBottom->Size = System::Drawing::Size(1100, 44);

            this->lblStatus->AutoSize = false;
            this->lblStatus->Location = System::Drawing::Point(12, 12);
            this->lblStatus->Size = System::Drawing::Size(800, 20);

            this->btnClose->Location = System::Drawing::Point(1000, 8);
            this->btnClose->Size = System::Drawing::Size(90, 28);
            this->btnClose->Text = L"Đóng";
            this->btnClose->Anchor = static_cast<System::Windows::Forms::AnchorStyles>(
                System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Right);

            this->AutoScaleDimensions = System::Drawing::SizeF(7, 15);
            this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
            this->ClientSize = System::Drawing::Size(1100, 700);
            this->Controls->Add(this->dgvRegions);
            this->Controls->Add(this->pnlCanvas);
            this->Controls->Add(this->pnlTop);
            this->Controls->Add(this->pnlBottom);
            this->Name = L"HeatmapForm";
            this->Text = L"Memory Heatmap — CheatVN";
            this->StartPosition = System::Windows::Forms::FormStartPosition::CenterParent;

            this->pnlTop->ResumeLayout(false);
            this->pnlTop->PerformLayout();
            (cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->dgvRegions))->EndInit();
            this->pnlBottom->ResumeLayout(false);
            this->pnlBottom->PerformLayout();
            this->ResumeLayout(false);
        }
#pragma endregion

        void OnFormLoad(Object^ sender, EventArgs^ e) {
            this->lblTitle->Text = String::Format(L"Memory Heatmap — {0}", _processName);
            LoadRegions();
        }

        void OnRefreshClick(Object^ sender, EventArgs^ e) {
            LoadRegions();
        }

        void OnCloseClick(Object^ sender, EventArgs^ e) {
            this->Close();
        }

        void LoadRegions() {
            _regions = ScannerBridge::EnumerateRegions(_hProcess);

            // Sort theo BaseAddress để vẽ heatmap dọc theo address space
            _regions->Sort(gcnew Comparison<ManagedMemoryRegion^>(
                &HeatmapForm::CompareByAddress));

            // Bind DGV (sort lại theo size cho readable nhất)
            auto dgvList = gcnew List<ManagedMemoryRegion^>(_regions);
            dgvList->Sort(gcnew Comparison<ManagedMemoryRegion^>(
                &HeatmapForm::CompareBySizeDesc));
            this->dgvRegions->DataSource = dgvList;

            // Compute total bytes + status
            UInt64 totalBytes = 0;
            int imageCount = 0, privateCount = 0, mappedCount = 0;
            for each (ManagedMemoryRegion^ r in _regions) {
                totalBytes += r->Size;
                if (r->IsImage) imageCount++;
                else if (r->IsPrivate) privateCount++;
                else if (r->IsMapped) mappedCount++;
            }

            this->lblStatus->Text = String::Format(
                L"Tổng: {0} regions · {1:F1} MB · {2} Image, {3} Private, {4} Mapped",
                _regions->Count,
                totalBytes / 1024.0 / 1024.0,
                imageCount, privateCount, mappedCount);

            this->pnlCanvas->Invalidate();  // force repaint
        }

        // Static comparison helpers (managed methods cần signature đặc biệt)
        static int CompareByAddress(ManagedMemoryRegion^ a, ManagedMemoryRegion^ b) {
            if (a->BaseAddress < b->BaseAddress) return -1;
            if (a->BaseAddress > b->BaseAddress) return 1;
            return 0;
        }
        static int CompareBySizeDesc(ManagedMemoryRegion^ a, ManagedMemoryRegion^ b) {
            if (a->Size > b->Size) return -1;
            if (a->Size < b->Size) return 1;
            return 0;
        }

        // =================================================================
        //  OnCanvasPaint — vẽ heatmap ribbon
        // =================================================================
        // Vẽ horizontal bar, mỗi region 1 rectangle, width tỷ lệ với log(size)
        // để region nhỏ vẫn nhìn thấy. Màu theo type:
        //   Image  = purple (DarkTheme::ValueColor)
        //   Private = blue (DarkTheme::Accent)
        //   Mapped = orange (DarkTheme::ChangedColor)
        //   Other  = gray (DarkTheme::Border)
        void OnCanvasPaint(Object^ sender, PaintEventArgs^ e) {
            if (_regions == nullptr || _regions->Count == 0) return;

            auto g = e->Graphics;
            g->SmoothingMode = SmoothingMode::AntiAlias;

            int canvasW = this->pnlCanvas->ClientSize.Width - 20;
            int canvasH = this->pnlCanvas->ClientSize.Height - 20;
            int startX = 10;
            int startY = 10;

            // Dùng log scale cho width (handle variation 1KB to 1GB)
            // logSize_i / sumLogSizes * canvasW
            double sumLogSize = 0;
            for each (ManagedMemoryRegion^ r in _regions) {
                sumLogSize += Math::Log((double)r->Size + 1.0);
            }
            if (sumLogSize <= 0) return;

            double xCursor = (double)startX;
            for each (ManagedMemoryRegion^ r in _regions) {
                double w = Math::Log((double)r->Size + 1.0) / sumLogSize * canvasW;
                if (w < 1.0) w = 1.0;  // ensure visible

                // Chọn màu theo type
                Color color;
                if (r->IsImage)        color = DarkTheme::ValueColor;
                else if (r->IsPrivate) color = DarkTheme::Accent;
                else if (r->IsMapped)  color = DarkTheme::ChangedColor;
                else                   color = DarkTheme::Border;

                // Nếu read-only, giảm độ sáng (alpha)
                if (!r->IsWritable) {
                    color = Color::FromArgb(160, color.R, color.G, color.B);
                }

                auto brush = gcnew SolidBrush(color);
                g->FillRectangle(brush, (float)xCursor, (float)startY,
                                  (float)w, (float)canvasH);
                delete brush;

                xCursor += w;
            }

            // Vẽ border ngoài
            auto borderPen = gcnew Pen(DarkTheme::Border);
            g->DrawRectangle(borderPen, startX, startY, canvasW, canvasH);
            delete borderPen;
        }
    };

} // namespace GUI

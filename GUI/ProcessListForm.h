#pragma once

#include "DarkTheme.h"
#include "ProcessManagerBridge.h"

namespace GUI {

    using namespace System;
    using namespace System::ComponentModel;
    using namespace System::Collections;
    using namespace System::Collections::Generic;
    using namespace System::Windows::Forms;
    using namespace System::Data;
    using namespace System::Drawing;

    /// <summary>
    /// ProcessListForm — Dialog cho user chọn tiến trình mục tiêu.
    /// Caller dùng pattern:
    ///     auto dlg = gcnew ProcessListForm();
    ///     if (dlg->ShowDialog(this) == DialogResult::OK) {
    ///         auto info = dlg->SelectedProcess;  // dùng info...
    ///     }
    /// </summary>
    public ref class ProcessListForm : public System::Windows::Forms::Form
    {
    public:
        ProcessListForm(void)
        {
            // BƯỚC 1: Designer-generated layout (chỉ có literal value, không
            //         có reference đến DarkTheme/code custom — để Designer
            //         có thể parse và mở View Designer mượt).
            InitializeComponent();

            // BƯỚC 2: Áp dark theme cho toàn bộ form
            DarkTheme::Apply(this);

            // BƯỚC 3: Custom styling cho các control đặc biệt
            // (làm sau Apply để có thể OVERRIDE màu mặc định)
            SetupCustomStyling();

            // BƯỚC 4: Wire event handlers
            this->Load += gcnew EventHandler(this, &ProcessListForm::OnFormLoad);
            this->timerRefresh->Tick += gcnew EventHandler(this, &ProcessListForm::OnTimerTick);
            this->txtSearch->TextChanged += gcnew EventHandler(this, &ProcessListForm::OnFilterChanged);
            this->chkHideSystem->CheckedChanged += gcnew EventHandler(this, &ProcessListForm::OnFilterChanged);
            this->chkX64Only->CheckedChanged += gcnew EventHandler(this, &ProcessListForm::OnFilterChanged);
            this->dgvProcesses->SelectionChanged += gcnew EventHandler(this, &ProcessListForm::OnSelectionChanged);
            this->btnRefresh->Click += gcnew EventHandler(this, &ProcessListForm::OnRefreshClick);
        }

        /// Process được user chọn. Đọc sau khi ShowDialog trả về OK.
        property ManagedProcessInfo^ SelectedProcess;

    protected:
        ~ProcessListForm()
        {
            if (components) delete components;
        }

        // Override OnFormClosing để lưu process được chọn vào property
        // trước khi form đóng. Caller sẽ đọc property này.
        virtual void OnFormClosing(FormClosingEventArgs^ e) override {
            if (this->DialogResult == System::Windows::Forms::DialogResult::OK
                && this->dgvProcesses->SelectedRows->Count > 0) {
                auto row = this->dgvProcesses->SelectedRows[0];
                this->SelectedProcess =
                    safe_cast<ManagedProcessInfo^>(row->DataBoundItem);
            }
            Form::OnFormClosing(e);
        }

    private:
        // Lưu list process gốc (chưa filter). Reload mỗi 2s.
        List<ManagedProcessInfo^>^ _allProcesses;

        // Container cho Timer + các component vô hình
        System::ComponentModel::IContainer^ components;

        // ─── Member fields cho các control ───────────────────────
        System::Windows::Forms::Panel^ pnlToolbar;
        System::Windows::Forms::Label^ lblSearch;
        System::Windows::Forms::TextBox^ txtSearch;
        System::Windows::Forms::CheckBox^ chkHideSystem;
        System::Windows::Forms::CheckBox^ chkX64Only;
        System::Windows::Forms::DataGridView^ dgvProcesses;
        System::Windows::Forms::DataGridViewTextBoxColumn^ colPid;
        System::Windows::Forms::DataGridViewTextBoxColumn^ colName;
        System::Windows::Forms::DataGridViewTextBoxColumn^ colBitness;
        System::Windows::Forms::DataGridViewTextBoxColumn^ colRam;
        System::Windows::Forms::DataGridViewTextBoxColumn^ colStatus;
        System::Windows::Forms::Panel^ pnlButtons;
        System::Windows::Forms::Button^ btnRefresh;
        System::Windows::Forms::Button^ btnCancel;
        System::Windows::Forms::Button^ btnOpen;
        System::Windows::Forms::StatusStrip^ statusStrip;
        System::Windows::Forms::ToolStripStatusLabel^ lblStatusCount;
        System::Windows::Forms::ToolStripStatusLabel^ lblStatusAutoRefresh;
        System::Windows::Forms::Timer^ timerRefresh;

        // =================================================================
        //  CUSTOM STYLING (ngoài InitializeComponent để Designer không xóa)
        // =================================================================
        // Đặt sau DarkTheme::Apply để override các thiết lập "chung chung"
        // bằng styling đặc biệt cho từng button (vd: btnOpen màu xanh accent).
        void SetupCustomStyling() {
            // Panel toolbar + buttons: màu hơi sáng hơn form (tạo layer)
            this->pnlToolbar->BackColor = DarkTheme::PanelSurface;
            this->pnlButtons->BackColor = DarkTheme::PanelSurface;

            // btnOpen là PRIMARY button → màu accent (xanh dương)
            this->btnOpen->BackColor = DarkTheme::Accent;
            this->btnOpen->ForeColor = Color::White;
            this->btnOpen->FlatAppearance->BorderColor = DarkTheme::Accent;
            // Khi hover, tăng độ sáng nhẹ (Lighter)
            this->btnOpen->FlatAppearance->MouseOverBackColor =
                Color::FromArgb(120, 188, 255);
        }

#pragma region Windows Form Designer generated code
        /// <summary>
        /// Required method for Designer support — do not modify with code
        /// editor anything that references custom code (chỉ literal value).
        /// </summary>
        void InitializeComponent(void)
        {
            this->components = gcnew System::ComponentModel::Container();
            this->pnlToolbar = gcnew System::Windows::Forms::Panel();
            this->lblSearch = gcnew System::Windows::Forms::Label();
            this->txtSearch = gcnew System::Windows::Forms::TextBox();
            this->chkHideSystem = gcnew System::Windows::Forms::CheckBox();
            this->chkX64Only = gcnew System::Windows::Forms::CheckBox();
            this->dgvProcesses = gcnew System::Windows::Forms::DataGridView();
            this->colPid = gcnew System::Windows::Forms::DataGridViewTextBoxColumn();
            this->colName = gcnew System::Windows::Forms::DataGridViewTextBoxColumn();
            this->colBitness = gcnew System::Windows::Forms::DataGridViewTextBoxColumn();
            this->colRam = gcnew System::Windows::Forms::DataGridViewTextBoxColumn();
            this->colStatus = gcnew System::Windows::Forms::DataGridViewTextBoxColumn();
            this->pnlButtons = gcnew System::Windows::Forms::Panel();
            this->btnRefresh = gcnew System::Windows::Forms::Button();
            this->btnCancel = gcnew System::Windows::Forms::Button();
            this->btnOpen = gcnew System::Windows::Forms::Button();
            this->statusStrip = gcnew System::Windows::Forms::StatusStrip();
            this->lblStatusCount = gcnew System::Windows::Forms::ToolStripStatusLabel();
            this->lblStatusAutoRefresh = gcnew System::Windows::Forms::ToolStripStatusLabel();
            this->timerRefresh = gcnew System::Windows::Forms::Timer(this->components);
            this->pnlToolbar->SuspendLayout();
            (cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->dgvProcesses))->BeginInit();
            this->pnlButtons->SuspendLayout();
            this->statusStrip->SuspendLayout();
            this->SuspendLayout();
            //
            // pnlToolbar
            //
            this->pnlToolbar->Controls->Add(this->chkX64Only);
            this->pnlToolbar->Controls->Add(this->chkHideSystem);
            this->pnlToolbar->Controls->Add(this->txtSearch);
            this->pnlToolbar->Controls->Add(this->lblSearch);
            this->pnlToolbar->Dock = System::Windows::Forms::DockStyle::Top;
            this->pnlToolbar->Location = System::Drawing::Point(0, 0);
            this->pnlToolbar->Name = L"pnlToolbar";
            this->pnlToolbar->Size = System::Drawing::Size(700, 44);
            this->pnlToolbar->TabIndex = 0;
            //
            // lblSearch
            //
            this->lblSearch->AutoSize = true;
            this->lblSearch->Location = System::Drawing::Point(12, 14);
            this->lblSearch->Name = L"lblSearch";
            this->lblSearch->Size = System::Drawing::Size(31, 15);
            this->lblSearch->TabIndex = 0;
            this->lblSearch->Text = L"Tìm:";
            //
            // txtSearch
            //
            this->txtSearch->Location = System::Drawing::Point(50, 11);
            this->txtSearch->Name = L"txtSearch";
            this->txtSearch->Size = System::Drawing::Size(320, 23);
            this->txtSearch->TabIndex = 1;
            //
            // chkHideSystem
            //
            this->chkHideSystem->AutoSize = true;
            this->chkHideSystem->Checked = true;
            this->chkHideSystem->CheckState = System::Windows::Forms::CheckState::Checked;
            this->chkHideSystem->Location = System::Drawing::Point(385, 13);
            this->chkHideSystem->Name = L"chkHideSystem";
            this->chkHideSystem->Size = System::Drawing::Size(130, 19);
            this->chkHideSystem->TabIndex = 2;
            this->chkHideSystem->Text = L"Ẩn system process";
            //
            // chkX64Only
            //
            this->chkX64Only->AutoSize = true;
            this->chkX64Only->Location = System::Drawing::Point(530, 13);
            this->chkX64Only->Name = L"chkX64Only";
            this->chkX64Only->Size = System::Drawing::Size(70, 19);
            this->chkX64Only->TabIndex = 3;
            this->chkX64Only->Text = L"Chỉ x64";
            //
            // dgvProcesses
            //
            this->dgvProcesses->AllowUserToAddRows = false;
            this->dgvProcesses->AllowUserToDeleteRows = false;
            this->dgvProcesses->AllowUserToResizeRows = false;
            // Tắt auto-generate columns: chỉ dùng 5 cột mình định nghĩa thủ công
            // bên dưới, không cho DGV tự tạo cột từ public property của object.
            this->dgvProcesses->AutoGenerateColumns = false;
            this->dgvProcesses->ColumnHeadersHeight = 30;
            this->dgvProcesses->ColumnHeadersHeightSizeMode =
                System::Windows::Forms::DataGridViewColumnHeadersHeightSizeMode::DisableResizing;
            this->dgvProcesses->Columns->AddRange(
                gcnew cli::array<System::Windows::Forms::DataGridViewColumn^>(5) {
                    this->colPid, this->colName, this->colBitness,
                    this->colRam, this->colStatus
                });
            this->dgvProcesses->Dock = System::Windows::Forms::DockStyle::Fill;
            this->dgvProcesses->Location = System::Drawing::Point(0, 44);
            this->dgvProcesses->MultiSelect = false;
            this->dgvProcesses->Name = L"dgvProcesses";
            this->dgvProcesses->ReadOnly = true;
            this->dgvProcesses->RowHeadersVisible = false;
            this->dgvProcesses->SelectionMode = System::Windows::Forms::DataGridViewSelectionMode::FullRowSelect;
            this->dgvProcesses->Size = System::Drawing::Size(700, 404);
            this->dgvProcesses->TabIndex = 1;
            //
            // colPid
            //
            this->colPid->DataPropertyName = L"Pid";
            this->colPid->HeaderText = L"PID";
            this->colPid->Name = L"colPid";
            this->colPid->ReadOnly = true;
            this->colPid->Width = 70;
            //
            // colName
            //
            this->colName->AutoSizeMode = System::Windows::Forms::DataGridViewAutoSizeColumnMode::Fill;
            this->colName->DataPropertyName = L"Name";
            this->colName->HeaderText = L"Tên tiến trình";
            this->colName->Name = L"colName";
            this->colName->ReadOnly = true;
            //
            // colBitness
            //
            this->colBitness->DataPropertyName = L"Bitness";
            this->colBitness->HeaderText = L"Bit";
            this->colBitness->Name = L"colBitness";
            this->colBitness->ReadOnly = true;
            this->colBitness->Width = 50;
            //
            // colRam
            //
            this->colRam->DataPropertyName = L"RamDisplay";
            this->colRam->HeaderText = L"RAM";
            this->colRam->Name = L"colRam";
            this->colRam->ReadOnly = true;
            this->colRam->Width = 90;
            //
            // colStatus
            //
            this->colStatus->DataPropertyName = L"Status";
            this->colStatus->HeaderText = L"Trạng thái";
            this->colStatus->Name = L"colStatus";
            this->colStatus->ReadOnly = true;
            this->colStatus->Width = 90;
            //
            // pnlButtons
            //
            this->pnlButtons->Controls->Add(this->btnRefresh);
            this->pnlButtons->Controls->Add(this->btnCancel);
            this->pnlButtons->Controls->Add(this->btnOpen);
            this->pnlButtons->Dock = System::Windows::Forms::DockStyle::Bottom;
            this->pnlButtons->Location = System::Drawing::Point(0, 448);
            this->pnlButtons->Name = L"pnlButtons";
            this->pnlButtons->Size = System::Drawing::Size(700, 50);
            this->pnlButtons->TabIndex = 2;
            //
            // btnRefresh
            //
            this->btnRefresh->Anchor =
                static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Right));
            this->btnRefresh->Location = System::Drawing::Point(340, 12);
            this->btnRefresh->Name = L"btnRefresh";
            this->btnRefresh->Size = System::Drawing::Size(90, 28);
            this->btnRefresh->TabIndex = 0;
            this->btnRefresh->Text = L"Làm mới";
            this->btnRefresh->UseVisualStyleBackColor = true;
            //
            // btnCancel
            //
            this->btnCancel->Anchor =
                static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Right));
            this->btnCancel->DialogResult = System::Windows::Forms::DialogResult::Cancel;
            this->btnCancel->Location = System::Drawing::Point(440, 12);
            this->btnCancel->Name = L"btnCancel";
            this->btnCancel->Size = System::Drawing::Size(80, 28);
            this->btnCancel->TabIndex = 1;
            this->btnCancel->Text = L"Hủy";
            this->btnCancel->UseVisualStyleBackColor = true;
            //
            // btnOpen
            //
            this->btnOpen->Anchor =
                static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Right));
            this->btnOpen->DialogResult = System::Windows::Forms::DialogResult::OK;
            this->btnOpen->Enabled = false;
            this->btnOpen->Location = System::Drawing::Point(530, 12);
            this->btnOpen->Name = L"btnOpen";
            this->btnOpen->Size = System::Drawing::Size(150, 28);
            this->btnOpen->TabIndex = 2;
            this->btnOpen->Text = L"Mở tiến trình";
            this->btnOpen->UseVisualStyleBackColor = true;
            //
            // statusStrip
            //
            this->statusStrip->Items->AddRange(
                gcnew cli::array<System::Windows::Forms::ToolStripItem^>(2) {
                    this->lblStatusCount, this->lblStatusAutoRefresh
                });
            this->statusStrip->Location = System::Drawing::Point(0, 498);
            this->statusStrip->Name = L"statusStrip";
            this->statusStrip->Size = System::Drawing::Size(700, 22);
            this->statusStrip->SizingGrip = false;
            this->statusStrip->TabIndex = 3;
            //
            // lblStatusCount
            //
            this->lblStatusCount->Name = L"lblStatusCount";
            this->lblStatusCount->Size = System::Drawing::Size(575, 17);
            this->lblStatusCount->Spring = true;
            this->lblStatusCount->Text = L"0 tiến trình";
            this->lblStatusCount->TextAlign = System::Drawing::ContentAlignment::MiddleLeft;
            //
            // lblStatusAutoRefresh
            //
            this->lblStatusAutoRefresh->Name = L"lblStatusAutoRefresh";
            this->lblStatusAutoRefresh->Size = System::Drawing::Size(110, 17);
            this->lblStatusAutoRefresh->Text = L"Auto refresh 2s";
            //
            // timerRefresh
            //
            this->timerRefresh->Enabled = true;
            this->timerRefresh->Interval = 2000;
            //
            // ProcessListForm
            //
            this->AcceptButton = this->btnOpen;
            this->AutoScaleDimensions = System::Drawing::SizeF(7, 15);
            this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
            this->CancelButton = this->btnCancel;
            this->ClientSize = System::Drawing::Size(700, 520);
            this->Controls->Add(this->dgvProcesses);
            this->Controls->Add(this->pnlToolbar);
            this->Controls->Add(this->pnlButtons);
            this->Controls->Add(this->statusStrip);
            this->FormBorderStyle = System::Windows::Forms::FormBorderStyle::FixedDialog;
            this->MaximizeBox = false;
            this->MinimizeBox = false;
            this->Name = L"ProcessListForm";
            this->ShowInTaskbar = false;
            this->StartPosition = System::Windows::Forms::FormStartPosition::CenterParent;
            this->Text = L"Chọn tiến trình mục tiêu";
            this->pnlToolbar->ResumeLayout(false);
            this->pnlToolbar->PerformLayout();
            (cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->dgvProcesses))->EndInit();
            this->pnlButtons->ResumeLayout(false);
            this->statusStrip->ResumeLayout(false);
            this->statusStrip->PerformLayout();
            this->ResumeLayout(false);
            this->PerformLayout();
        }
#pragma endregion

        // =================================================================
        //  EVENT HANDLERS
        // =================================================================

        void OnFormLoad(Object^ sender, EventArgs^ e) {
            LoadProcessList();
        }

        void OnTimerTick(Object^ sender, EventArgs^ e) {
            LoadProcessList();
        }

        void OnRefreshClick(Object^ sender, EventArgs^ e) {
            LoadProcessList();
        }

        void OnFilterChanged(Object^ sender, EventArgs^ e) {
            ApplyFilter();
        }

        void OnSelectionChanged(Object^ sender, EventArgs^ e) {
            this->btnOpen->Enabled = (this->dgvProcesses->SelectedRows->Count > 0);
        }

        // =================================================================
        //  HELPERS
        // =================================================================

        /// Gọi Bridge → Native → lấy list process, lưu vào _allProcesses
        void LoadProcessList() {
            // Save PID đang select để restore sau load (UX nhẹ nhàng)
            UInt32 previousSelectedPid = 0;
            if (this->dgvProcesses->SelectedRows->Count > 0) {
                auto row = this->dgvProcesses->SelectedRows[0];
                if (row->DataBoundItem != nullptr) {
                    previousSelectedPid =
                        safe_cast<ManagedProcessInfo^>(row->DataBoundItem)->Pid;
                }
            }

            _allProcesses = ProcessManagerBridge::EnumerateProcesses();
            ApplyFilter();

            if (previousSelectedPid != 0) {
                for (int i = 0; i < this->dgvProcesses->Rows->Count; ++i) {
                    auto info = safe_cast<ManagedProcessInfo^>(
                        this->dgvProcesses->Rows[i]->DataBoundItem);
                    if (info->Pid == previousSelectedPid) {
                        this->dgvProcesses->ClearSelection();
                        this->dgvProcesses->Rows[i]->Selected = true;
                        break;
                    }
                }
            }
        }

        /// Áp filter lên _allProcesses và binding lên DGV
        void ApplyFilter() {
            if (_allProcesses == nullptr) return;

            String^ search = this->txtSearch->Text->Trim()->ToLowerInvariant();
            bool hideSystem = this->chkHideSystem->Checked;
            bool x64Only = this->chkX64Only->Checked;

            auto filtered = gcnew List<ManagedProcessInfo^>();
            for each (ManagedProcessInfo^ p in _allProcesses) {
                if (search->Length > 0) {
                    bool matchName = p->Name->ToLowerInvariant()->Contains(search);
                    bool matchPid = p->Pid.ToString()->Contains(search);
                    if (!matchName && !matchPid) continue;
                }
                if (hideSystem && !p->IsAccessible) continue;
                if (x64Only && !p->Is64Bit) continue;
                filtered->Add(p);
            }

            this->dgvProcesses->DataSource = filtered;
            this->lblStatusCount->Text = String::Format(
                L"{0} tiến trình (lọc từ {1})",
                filtered->Count, _allProcesses->Count);
        }
    };

} // namespace GUI

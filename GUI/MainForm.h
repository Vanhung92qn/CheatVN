#pragma once

#include "DarkTheme.h"
#include "ProcessListForm.h"
#include "ScannerBridge.h"

namespace GUI {

	using namespace System;
	using namespace System::ComponentModel;
	using namespace System::Collections;
	using namespace System::Collections::Generic;
	using namespace System::Windows::Forms;
	using namespace System::Data;
	using namespace System::Drawing;
	using namespace System::Diagnostics;

	/// <summary>
	/// MainForm — cửa sổ chính của CheatVN.
	/// Workflow:
	///   1. Bấm "Chọn tiến trình" → ProcessListForm → pick process
	///   2. Sau khi pick, mở handle PROCESS_VM_READ + PROCESS_VM_WRITE
	///   3. Nhập giá trị, chọn type + operator → Quét lần đầu → kết quả lên DGV
	///   4. Nhập giá trị mới + operator → Quét lại → lọc kết quả
	///   5. (Phase 3) Edit / Freeze value
	/// </summary>
	public ref class MainForm : public System::Windows::Forms::Form
	{
	public:
		MainForm(void)
		{
			InitializeComponent();
			DarkTheme::Apply(this);
			SetupCustomStyling();
			PopulateComboBoxes();

			// Wire events
			this->btnChooseProcess->Click +=
				gcnew EventHandler(this, &MainForm::OnChooseProcessClick);
			this->btnFirstScan->Click +=
				gcnew EventHandler(this, &MainForm::OnFirstScanClick);
			this->btnNextScan->Click +=
				gcnew EventHandler(this, &MainForm::OnNextScanClick);
			this->btnReset->Click +=
				gcnew EventHandler(this, &MainForm::OnResetClick);
			this->btnEditValue->Click +=
				gcnew EventHandler(this, &MainForm::OnEditValueClick);
			this->timerRefresh->Tick +=
				gcnew EventHandler(this, &MainForm::OnRefreshTick);
			this->timerFreeze->Tick +=
				gcnew EventHandler(this, &MainForm::OnFreezeTick);

			// Wire DGV events cho freeze checkbox
			this->dgvResults->CellValueChanged +=
				gcnew DataGridViewCellEventHandler(this, &MainForm::OnCellValueChanged);
			this->dgvResults->CurrentCellDirtyStateChanged +=
				gcnew EventHandler(this, &MainForm::OnCellDirtyStateChanged);

			this->btnSaveSession->Click +=
				gcnew EventHandler(this, &MainForm::OnSaveSessionClick);
			this->btnLoadSession->Click +=
				gcnew EventHandler(this, &MainForm::OnLoadSessionClick);

			// State khởi đầu: chưa attach process → tắt scan UI
			UpdateScanUIState();
		}

	protected:
		~MainForm()
		{
			// Đóng handle process khi form đóng
			if (_processHandle != IntPtr::Zero) {
				ProcessManagerBridge::CloseHandle(_processHandle);
				_processHandle = IntPtr::Zero;
			}
			if (components) delete components;
		}

	private:
		// ─── State ───────────────────────────────────────────────
		ManagedProcessInfo^ _currentProcess;   // process đang attach (null nếu chưa)
		IntPtr _processHandle;                  // HANDLE đang giữ (zero nếu chưa mở)
		List<ManagedScanResult^>^ _results;     // kết quả scan hiện tại

		System::ComponentModel::IContainer^ components;

		// ─── Controls — toolbar process ─────────────────────────
		System::Windows::Forms::Panel^ pnlProcess;
		System::Windows::Forms::Button^ btnChooseProcess;
		System::Windows::Forms::Label^ lblCurrentProcess;
		System::Windows::Forms::Button^ btnLoadSession;
		System::Windows::Forms::Button^ btnSaveSession;

		// ─── Controls — scan input ──────────────────────────────
		System::Windows::Forms::Panel^ pnlScanInput;
		System::Windows::Forms::Label^ lblValue;
		System::Windows::Forms::TextBox^ txtValue;
		System::Windows::Forms::Label^ lblValueType;
		System::Windows::Forms::ComboBox^ cmbValueType;
		System::Windows::Forms::Label^ lblOperator;
		System::Windows::Forms::ComboBox^ cmbOperator;
		System::Windows::Forms::Button^ btnFirstScan;
		System::Windows::Forms::Button^ btnNextScan;
		System::Windows::Forms::Button^ btnReset;
		System::Windows::Forms::Button^ btnEditValue;
		System::Windows::Forms::Label^ lblResultCount;
		System::Windows::Forms::Timer^ timerRefresh;

		// ─── Controls — results ─────────────────────────────────
		System::Windows::Forms::DataGridView^ dgvResults;
		System::Windows::Forms::DataGridViewCheckBoxColumn^ colFrozen;
		System::Windows::Forms::DataGridViewTextBoxColumn^ colAddress;
		System::Windows::Forms::DataGridViewTextBoxColumn^ colCurrent;
		System::Windows::Forms::DataGridViewTextBoxColumn^ colPrevious;
		System::Windows::Forms::Timer^ timerFreeze;

		// ─── Status ─────────────────────────────────────────────
		System::Windows::Forms::StatusStrip^ statusStrip;
		System::Windows::Forms::ToolStripStatusLabel^ lblStatus;
		System::Windows::Forms::ToolStripStatusLabel^ lblScanTime;

#pragma region Windows Form Designer generated code
		void InitializeComponent(void)
		{
			this->components = gcnew System::ComponentModel::Container();
			this->pnlProcess = gcnew System::Windows::Forms::Panel();
			this->btnChooseProcess = gcnew System::Windows::Forms::Button();
			this->lblCurrentProcess = gcnew System::Windows::Forms::Label();
			this->btnLoadSession = gcnew System::Windows::Forms::Button();
			this->btnSaveSession = gcnew System::Windows::Forms::Button();
			this->pnlScanInput = gcnew System::Windows::Forms::Panel();
			this->lblValue = gcnew System::Windows::Forms::Label();
			this->txtValue = gcnew System::Windows::Forms::TextBox();
			this->lblValueType = gcnew System::Windows::Forms::Label();
			this->cmbValueType = gcnew System::Windows::Forms::ComboBox();
			this->lblOperator = gcnew System::Windows::Forms::Label();
			this->cmbOperator = gcnew System::Windows::Forms::ComboBox();
			this->btnFirstScan = gcnew System::Windows::Forms::Button();
			this->btnNextScan = gcnew System::Windows::Forms::Button();
			this->btnReset = gcnew System::Windows::Forms::Button();
			this->btnEditValue = gcnew System::Windows::Forms::Button();
			this->lblResultCount = gcnew System::Windows::Forms::Label();
			this->timerRefresh = gcnew System::Windows::Forms::Timer(this->components);
			this->dgvResults = gcnew System::Windows::Forms::DataGridView();
			this->colFrozen = gcnew System::Windows::Forms::DataGridViewCheckBoxColumn();
			this->colAddress = gcnew System::Windows::Forms::DataGridViewTextBoxColumn();
			this->colCurrent = gcnew System::Windows::Forms::DataGridViewTextBoxColumn();
			this->colPrevious = gcnew System::Windows::Forms::DataGridViewTextBoxColumn();
			this->timerFreeze = gcnew System::Windows::Forms::Timer(this->components);
			this->statusStrip = gcnew System::Windows::Forms::StatusStrip();
			this->lblStatus = gcnew System::Windows::Forms::ToolStripStatusLabel();
			this->lblScanTime = gcnew System::Windows::Forms::ToolStripStatusLabel();

			this->pnlProcess->SuspendLayout();
			this->pnlScanInput->SuspendLayout();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->dgvResults))->BeginInit();
			this->statusStrip->SuspendLayout();
			this->SuspendLayout();

			// ─── pnlProcess ────────────────────────────────────
			this->pnlProcess->Controls->Add(this->btnSaveSession);
			this->pnlProcess->Controls->Add(this->btnLoadSession);
			this->pnlProcess->Controls->Add(this->lblCurrentProcess);
			this->pnlProcess->Controls->Add(this->btnChooseProcess);
			this->pnlProcess->Dock = System::Windows::Forms::DockStyle::Top;
			this->pnlProcess->Location = System::Drawing::Point(0, 0);
			this->pnlProcess->Name = L"pnlProcess";
			this->pnlProcess->Size = System::Drawing::Size(1240, 56);

			// ─── btnChooseProcess ──────────────────────────────
			this->btnChooseProcess->Location = System::Drawing::Point(12, 12);
			this->btnChooseProcess->Size = System::Drawing::Size(170, 32);
			this->btnChooseProcess->Name = L"btnChooseProcess";
			this->btnChooseProcess->Text = L"Chọn tiến trình";

			// ─── lblCurrentProcess ─────────────────────────────
			this->lblCurrentProcess->Location = System::Drawing::Point(196, 20);
			this->lblCurrentProcess->Size = System::Drawing::Size(800, 20);
			this->lblCurrentProcess->Name = L"lblCurrentProcess";
			this->lblCurrentProcess->Text = L"Chưa chọn tiến trình nào...";
			this->lblCurrentProcess->AutoSize = false;

			// ─── btnLoadSession ────────────────────────────────
			this->btnLoadSession->Location = System::Drawing::Point(1010, 12);
			this->btnLoadSession->Size = System::Drawing::Size(105, 32);
			this->btnLoadSession->Name = L"btnLoadSession";
			this->btnLoadSession->Text = L"Mở session";

			// ─── btnSaveSession ────────────────────────────────
			this->btnSaveSession->Location = System::Drawing::Point(1123, 12);
			this->btnSaveSession->Size = System::Drawing::Size(105, 32);
			this->btnSaveSession->Name = L"btnSaveSession";
			this->btnSaveSession->Text = L"Lưu session";

			// ─── pnlScanInput ──────────────────────────────────
			this->pnlScanInput->Controls->Add(this->lblResultCount);
			this->pnlScanInput->Controls->Add(this->btnEditValue);
			this->pnlScanInput->Controls->Add(this->btnReset);
			this->pnlScanInput->Controls->Add(this->btnNextScan);
			this->pnlScanInput->Controls->Add(this->btnFirstScan);
			this->pnlScanInput->Controls->Add(this->cmbOperator);
			this->pnlScanInput->Controls->Add(this->lblOperator);
			this->pnlScanInput->Controls->Add(this->cmbValueType);
			this->pnlScanInput->Controls->Add(this->lblValueType);
			this->pnlScanInput->Controls->Add(this->txtValue);
			this->pnlScanInput->Controls->Add(this->lblValue);
			this->pnlScanInput->Dock = System::Windows::Forms::DockStyle::Top;
			this->pnlScanInput->Location = System::Drawing::Point(0, 56);
			this->pnlScanInput->Name = L"pnlScanInput";
			this->pnlScanInput->Size = System::Drawing::Size(1240, 90);

			// ─── lblValue ──────────────────────────────────────
			this->lblValue->AutoSize = true;
			this->lblValue->Location = System::Drawing::Point(12, 18);
			this->lblValue->Name = L"lblValue";
			this->lblValue->Text = L"Giá trị:";

			// ─── txtValue ──────────────────────────────────────
			this->txtValue->Location = System::Drawing::Point(12, 38);
			this->txtValue->Size = System::Drawing::Size(160, 23);
			this->txtValue->Name = L"txtValue";
			this->txtValue->Text = L"0";

			// ─── lblValueType ──────────────────────────────────
			this->lblValueType->AutoSize = true;
			this->lblValueType->Location = System::Drawing::Point(190, 18);
			this->lblValueType->Name = L"lblValueType";
			this->lblValueType->Text = L"Kiểu dữ liệu:";

			// ─── cmbValueType ──────────────────────────────────
			this->cmbValueType->DropDownStyle = System::Windows::Forms::ComboBoxStyle::DropDownList;
			this->cmbValueType->Location = System::Drawing::Point(190, 38);
			this->cmbValueType->Size = System::Drawing::Size(140, 23);
			this->cmbValueType->Name = L"cmbValueType";

			// ─── lblOperator ───────────────────────────────────
			this->lblOperator->AutoSize = true;
			this->lblOperator->Location = System::Drawing::Point(346, 18);
			this->lblOperator->Name = L"lblOperator";
			this->lblOperator->Text = L"Phép so sánh:";

			// ─── cmbOperator ───────────────────────────────────
			this->cmbOperator->DropDownStyle = System::Windows::Forms::ComboBoxStyle::DropDownList;
			this->cmbOperator->Location = System::Drawing::Point(346, 38);
			this->cmbOperator->Size = System::Drawing::Size(160, 23);
			this->cmbOperator->Name = L"cmbOperator";

			// ─── btnFirstScan ──────────────────────────────────
			this->btnFirstScan->Location = System::Drawing::Point(524, 36);
			this->btnFirstScan->Size = System::Drawing::Size(130, 28);
			this->btnFirstScan->Name = L"btnFirstScan";
			this->btnFirstScan->Text = L"Quét lần đầu";

			// ─── btnNextScan ───────────────────────────────────
			this->btnNextScan->Location = System::Drawing::Point(662, 36);
			this->btnNextScan->Size = System::Drawing::Size(120, 28);
			this->btnNextScan->Name = L"btnNextScan";
			this->btnNextScan->Text = L"Quét lại";

			// ─── btnReset ──────────────────────────────────────
			this->btnReset->Location = System::Drawing::Point(790, 36);
			this->btnReset->Size = System::Drawing::Size(90, 28);
			this->btnReset->Name = L"btnReset";
			this->btnReset->Text = L"Reset";

			// ─── btnEditValue ──────────────────────────────────
			// Ghi giá trị mới (từ txtValue) vào địa chỉ đang được chọn ở DGV.
			// Dùng để hack value: chọn row, gõ giá trị mới, bấm Sửa giá trị.
			this->btnEditValue->Location = System::Drawing::Point(890, 36);
			this->btnEditValue->Size = System::Drawing::Size(140, 28);
			this->btnEditValue->Name = L"btnEditValue";
			this->btnEditValue->Text = L"Sửa giá trị";

			// ─── lblResultCount ────────────────────────────────
			this->lblResultCount->AutoSize = false;
			this->lblResultCount->Location = System::Drawing::Point(1040, 42);
			this->lblResultCount->Size = System::Drawing::Size(180, 18);
			this->lblResultCount->Name = L"lblResultCount";
			this->lblResultCount->Text = L"Chưa quét";
			this->lblResultCount->TextAlign = System::Drawing::ContentAlignment::MiddleLeft;

			// ─── timerRefresh ──────────────────────────────────
			// Tick mỗi 500ms — gọi ScannerBridge::RefreshValues để cập nhật
			// CurrentRaw của các kết quả, force DGV redraw.
			// → Khi target process đổi giá trị, DGV hiển thị live ngay.
			this->timerRefresh->Interval = 500;
			this->timerRefresh->Enabled = true;

			// ─── timerFreeze ───────────────────────────────────
			// Tick mỗi 50ms — duyệt _results, ghi FrozenRaw vào các địa chỉ
			// có IsFrozen=true. 50ms = 20 lần/giây, đủ "khóa cứng" trước
			// hầu hết app cố đổi value.
			this->timerFreeze->Interval = 50;
			this->timerFreeze->Enabled = true;

			// ─── dgvResults ────────────────────────────────────
			this->dgvResults->AllowUserToAddRows = false;
			this->dgvResults->AllowUserToDeleteRows = false;
			this->dgvResults->AllowUserToResizeRows = false;
			this->dgvResults->AutoGenerateColumns = false;
			this->dgvResults->ColumnHeadersHeight = 30;
			this->dgvResults->ColumnHeadersHeightSizeMode =
				System::Windows::Forms::DataGridViewColumnHeadersHeightSizeMode::DisableResizing;
			this->dgvResults->Columns->AddRange(
				gcnew cli::array<System::Windows::Forms::DataGridViewColumn^>(4) {
					this->colFrozen, this->colAddress, this->colCurrent, this->colPrevious
				});
			this->dgvResults->Dock = System::Windows::Forms::DockStyle::Fill;
			this->dgvResults->Name = L"dgvResults";
			// ReadOnly = false để checkbox cột Frozen có thể tick được.
			// Các cột text khác đặt ReadOnly riêng (xem dưới).
			this->dgvResults->ReadOnly = false;
			this->dgvResults->RowHeadersVisible = false;
			this->dgvResults->SelectionMode = System::Windows::Forms::DataGridViewSelectionMode::FullRowSelect;
			this->dgvResults->MultiSelect = false;

			// ─── Columns ───────────────────────────────────────
			// colFrozen: checkbox cho user toggle freeze. Editable.
			this->colFrozen->DataPropertyName = L"IsFrozen";
			this->colFrozen->HeaderText = L"❄";
			this->colFrozen->Name = L"colFrozen";
			this->colFrozen->Width = 40;
			this->colFrozen->ReadOnly = false;
			this->colFrozen->Resizable = System::Windows::Forms::DataGridViewTriState::False;

			this->colAddress->DataPropertyName = L"AddressDisplay";
			this->colAddress->HeaderText = L"Địa chỉ";
			this->colAddress->Name = L"colAddress";
			this->colAddress->ReadOnly = true;
			this->colAddress->Width = 200;

			this->colCurrent->DataPropertyName = L"CurrentDisplay";
			this->colCurrent->HeaderText = L"Giá trị hiện tại";
			this->colCurrent->Name = L"colCurrent";
			this->colCurrent->ReadOnly = true;
			this->colCurrent->Width = 180;

			this->colPrevious->DataPropertyName = L"PreviousDisplay";
			this->colPrevious->HeaderText = L"Giá trị trước";
			this->colPrevious->Name = L"colPrevious";
			this->colPrevious->ReadOnly = true;
			this->colPrevious->AutoSizeMode = System::Windows::Forms::DataGridViewAutoSizeColumnMode::Fill;

			// ─── statusStrip ───────────────────────────────────
			this->statusStrip->Items->AddRange(
				gcnew cli::array<System::Windows::Forms::ToolStripItem^>(2) {
					this->lblStatus, this->lblScanTime
				});
			this->statusStrip->Name = L"statusStrip";
			this->statusStrip->SizingGrip = false;

			this->lblStatus->Name = L"lblStatus";
			this->lblStatus->Spring = true;
			this->lblStatus->Text = L"Sẵn sàng — hãy chọn 1 tiến trình";
			this->lblStatus->TextAlign = System::Drawing::ContentAlignment::MiddleLeft;

			this->lblScanTime->Name = L"lblScanTime";
			this->lblScanTime->Text = L"";

			// ─── Form ──────────────────────────────────────────
			this->AutoScaleDimensions = System::Drawing::SizeF(7, 15);
			this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
			this->ClientSize = System::Drawing::Size(1240, 720);
			// Thứ tự Controls->Add: bottom-edge controls TRƯỚC, fill SAU
			this->Controls->Add(this->statusStrip);
			this->Controls->Add(this->dgvResults);
			this->Controls->Add(this->pnlScanInput);
			this->Controls->Add(this->pnlProcess);
			this->Name = L"MainForm";
			this->Text = L"CheatVN — Trình phân tích bộ nhớ";
			this->StartPosition = System::Windows::Forms::FormStartPosition::CenterScreen;

			this->pnlProcess->ResumeLayout(false);
			this->pnlScanInput->ResumeLayout(false);
			this->pnlScanInput->PerformLayout();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->dgvResults))->EndInit();
			this->statusStrip->ResumeLayout(false);
			this->statusStrip->PerformLayout();
			this->ResumeLayout(false);
			this->PerformLayout();
		}
#pragma endregion

		// =================================================================
		//  Custom styling (sau DarkTheme::Apply để override theo ý)
		// =================================================================
		void SetupCustomStyling() {
			this->pnlProcess->BackColor = DarkTheme::PanelSurface;
			this->pnlScanInput->BackColor = DarkTheme::PanelSurface;

			// Primary button: btnFirstScan màu accent (xanh)
			this->btnFirstScan->BackColor = DarkTheme::Accent;
			this->btnFirstScan->ForeColor = Color::White;
			this->btnFirstScan->FlatAppearance->BorderColor = DarkTheme::Accent;
			this->btnFirstScan->FlatAppearance->MouseOverBackColor =
				Color::FromArgb(120, 188, 255);

			// btnEditValue: màu warning (cam) để nhấn mạnh "đây là ghi memory!"
			this->btnEditValue->BackColor = DarkTheme::ChangedColor;
			this->btnEditValue->ForeColor = Color::White;
			this->btnEditValue->FlatAppearance->BorderColor = DarkTheme::ChangedColor;
			this->btnEditValue->FlatAppearance->MouseOverBackColor =
				Color::FromArgb(255, 165, 130);

			// Result count label dùng màu accent
			this->lblResultCount->ForeColor = DarkTheme::Accent;
		}

		// =================================================================
		//  Populate ComboBoxes với value type + operator
		// =================================================================
		void PopulateComboBoxes() {
			// ComboBox cmbValueType: 6 kiểu dữ liệu cố định
			this->cmbValueType->Items->Add(L"Int32 (4 bytes)");
			this->cmbValueType->Items->Add(L"Int64 (8 bytes)");
			this->cmbValueType->Items->Add(L"Float (4 bytes)");
			this->cmbValueType->Items->Add(L"Double (8 bytes)");
			this->cmbValueType->Items->Add(L"Int16 (2 bytes)");
			this->cmbValueType->Items->Add(L"Int8 (1 byte)");
			this->cmbValueType->SelectedIndex = 0;  // Int32 mặc định

			// ComboBox cmbOperator: 12 phép so sánh
			// (vị trí trong list = index, không phải value của enum)
			this->cmbOperator->Items->Add(L"= Bằng");
			this->cmbOperator->Items->Add(L"≠ Khác");
			this->cmbOperator->Items->Add(L"> Lớn hơn");
			this->cmbOperator->Items->Add(L"≥ Lớn hơn hoặc bằng");
			this->cmbOperator->Items->Add(L"< Nhỏ hơn");
			this->cmbOperator->Items->Add(L"≤ Nhỏ hơn hoặc bằng");
			this->cmbOperator->Items->Add(L"~ Đã thay đổi");
			this->cmbOperator->Items->Add(L"= Không đổi");
			this->cmbOperator->Items->Add(L"↑ Tăng");
			this->cmbOperator->Items->Add(L"↓ Giảm");
			this->cmbOperator->SelectedIndex = 0;  // Exact mặc định
		}

		// Map combo selected index → ManagedValueType
		ManagedValueType SelectedValueType() {
			switch (this->cmbValueType->SelectedIndex) {
				case 0: return ManagedValueType::Int32;
				case 1: return ManagedValueType::Int64;
				case 2: return ManagedValueType::Float;
				case 3: return ManagedValueType::Double;
				case 4: return ManagedValueType::Int16;
				case 5: return ManagedValueType::Int8;
				default: return ManagedValueType::Int32;
			}
		}

		// Map combo selected index → ManagedScanOperator
		ManagedScanOperator SelectedOperator() {
			switch (this->cmbOperator->SelectedIndex) {
				case 0: return ManagedScanOperator::Exact;
				case 1: return ManagedScanOperator::NotEqual;
				case 2: return ManagedScanOperator::Greater;
				case 3: return ManagedScanOperator::GreaterOrEqual;
				case 4: return ManagedScanOperator::Less;
				case 5: return ManagedScanOperator::LessOrEqual;
				case 6: return ManagedScanOperator::Changed;
				case 7: return ManagedScanOperator::Unchanged;
				case 8: return ManagedScanOperator::Increased;
				case 9: return ManagedScanOperator::Decreased;
				default: return ManagedScanOperator::Exact;
			}
		}

		// =================================================================
		//  Cập nhật enable/disable controls theo state
		// =================================================================
		void UpdateScanUIState() {
			bool attached = (_processHandle != IntPtr::Zero);
			bool hasResults = (_results != nullptr && _results->Count > 0);

			this->txtValue->Enabled = attached;
			this->cmbValueType->Enabled = attached;
			this->cmbOperator->Enabled = attached;
			this->btnFirstScan->Enabled = attached;
			this->btnNextScan->Enabled = attached && hasResults;
			this->btnReset->Enabled = hasResults;
			this->btnEditValue->Enabled = attached && hasResults;
		}

		// =================================================================
		//  EVENT HANDLERS
		// =================================================================

		void OnChooseProcessClick(Object^ sender, EventArgs^ e) {
			auto dialog = gcnew ProcessListForm();
			auto result = dialog->ShowDialog(this);

			if (result != System::Windows::Forms::DialogResult::OK
				|| dialog->SelectedProcess == nullptr) {
				return;
			}

			// Đóng handle cũ (nếu đang attach process khác)
			if (_processHandle != IntPtr::Zero) {
				ProcessManagerBridge::CloseHandle(_processHandle);
				_processHandle = IntPtr::Zero;
			}

			// Mở handle vào process mới với quyền READ+WRITE
			_currentProcess = dialog->SelectedProcess;
			_processHandle = ProcessManagerBridge::OpenForReadWrite(_currentProcess->Pid);

			if (_processHandle == IntPtr::Zero) {
				MessageBox::Show(
					this,
					L"Không mở được tiến trình. Có thể bạn chưa chạy as Administrator,\n"
					L"hoặc tiến trình bị bảo vệ.",
					L"Lỗi",
					MessageBoxButtons::OK,
					MessageBoxIcon::Warning);
				_currentProcess = nullptr;
				this->lblCurrentProcess->Text = L"Chưa chọn tiến trình nào...";
				UpdateScanUIState();
				return;
			}

			// Hiển thị info process
			this->lblCurrentProcess->Text = String::Format(
				L"Đang attach: {0} (PID {1}, {2}) — RAM {3}",
				_currentProcess->Name,
				_currentProcess->Pid,
				_currentProcess->Bitness,
				_currentProcess->RamDisplay);
			this->lblCurrentProcess->ForeColor = DarkTheme::Success;

			// Reset results khi đổi process
			_results = nullptr;
			this->dgvResults->DataSource = nullptr;
			this->lblResultCount->Text = L"Chưa quét";

			this->lblStatus->Text = L"Đã attach. Nhập giá trị và bấm Quét lần đầu.";
			UpdateScanUIState();
		}

		void OnFirstScanClick(Object^ sender, EventArgs^ e) {
			ManagedValueType type = SelectedValueType();
			ManagedScanOperator op = SelectedOperator();

			// Parse giá trị từ TextBox
			Int64 targetRaw;
			if (!ScannerBridge::TryParseValue(this->txtValue->Text, type, targetRaw)) {
				MessageBox::Show(this,
					L"Giá trị không hợp lệ. Hãy nhập số đúng kiểu dữ liệu đã chọn.",
					L"Lỗi", MessageBoxButtons::OK, MessageBoxIcon::Warning);
				return;
			}

			// Disable UI trong khi scan (scan có thể tốn vài giây)
			this->Cursor = Cursors::WaitCursor;
			this->btnFirstScan->Enabled = false;
			this->btnNextScan->Enabled = false;
			this->lblStatus->Text = L"Đang quét...";
			Application::DoEvents();  // refresh UI

			// Đo thời gian scan
			auto sw = Stopwatch::StartNew();
			_results = ScannerBridge::FirstScan(_processHandle, type, targetRaw, op);
			sw->Stop();

			// Binding kết quả lên DGV
			this->dgvResults->DataSource = _results;

			this->lblResultCount->Text = String::Format(L"Tìm thấy: {0} địa chỉ", _results->Count);
			this->lblScanTime->Text = String::Format(L"Quét: {0:F2}s", sw->Elapsed.TotalSeconds);
			this->lblStatus->Text = L"Quét lần đầu xong. Có thể đổi giá trị + Quét lại để lọc.";
			this->Cursor = Cursors::Default;
			UpdateScanUIState();
		}

		void OnNextScanClick(Object^ sender, EventArgs^ e) {
			if (_results == nullptr || _results->Count == 0) return;

			ManagedValueType type = SelectedValueType();
			ManagedScanOperator op = SelectedOperator();

			// Với operator như Changed/Unchanged/Increased/Decreased,
			// giá trị target không quan trọng (set = 0).
			Int64 targetRaw = 0;
			bool needsValue =
				op == ManagedScanOperator::Exact ||
				op == ManagedScanOperator::NotEqual ||
				op == ManagedScanOperator::Greater ||
				op == ManagedScanOperator::GreaterOrEqual ||
				op == ManagedScanOperator::Less ||
				op == ManagedScanOperator::LessOrEqual ||
				op == ManagedScanOperator::IncreasedBy ||
				op == ManagedScanOperator::DecreasedBy;
			if (needsValue) {
				if (!ScannerBridge::TryParseValue(this->txtValue->Text, type, targetRaw)) {
					MessageBox::Show(this,
						L"Giá trị không hợp lệ.",
						L"Lỗi", MessageBoxButtons::OK, MessageBoxIcon::Warning);
					return;
				}
			}

			this->Cursor = Cursors::WaitCursor;
			this->btnFirstScan->Enabled = false;
			this->btnNextScan->Enabled = false;
			this->lblStatus->Text = L"Đang quét lại...";
			Application::DoEvents();

			auto sw = Stopwatch::StartNew();
			_results = ScannerBridge::NextScan(_processHandle, _results, type, targetRaw, op);
			sw->Stop();

			this->dgvResults->DataSource = _results;
			this->lblResultCount->Text = String::Format(L"Tìm thấy: {0} địa chỉ", _results->Count);
			this->lblScanTime->Text = String::Format(L"Quét: {0:F3}s", sw->Elapsed.TotalSeconds);
			this->lblStatus->Text = L"Quét lại xong.";
			this->Cursor = Cursors::Default;
			UpdateScanUIState();
		}

		void OnResetClick(Object^ sender, EventArgs^ e) {
			_results = nullptr;
			this->dgvResults->DataSource = nullptr;
			this->lblResultCount->Text = L"Chưa quét";
			this->lblScanTime->Text = L"";
			this->lblStatus->Text = L"Đã reset. Nhập giá trị mới và Quét lần đầu.";
			UpdateScanUIState();
		}

		// =================================================================
		//  OnEditValueClick — ghi giá trị mới vào địa chỉ đang chọn
		// =================================================================
		// Workflow:
		//   1. User chọn 1 row trong DGV (1 địa chỉ scan ra)
		//   2. User gõ giá trị mới vào txtValue
		//   3. Bấm Sửa giá trị → CheatVN gọi WriteProcessMemory ghi vào target
		//   4. Process target tức thì hiển thị giá trị mới (vì memory đã đổi)
		void OnEditValueClick(Object^ sender, EventArgs^ e) {
			if (this->dgvResults->SelectedRows->Count == 0) {
				MessageBox::Show(this,
					L"Hãy chọn 1 địa chỉ trong bảng trước khi Sửa.",
					L"Chưa chọn địa chỉ",
					MessageBoxButtons::OK, MessageBoxIcon::Information);
				return;
			}
			auto row = this->dgvResults->SelectedRows[0];
			if (row->DataBoundItem == nullptr) return;

			auto result = safe_cast<ManagedScanResult^>(row->DataBoundItem);

			// Parse giá trị mới từ txtValue, theo type của result
			Int64 valueRaw;
			if (!ScannerBridge::TryParseValue(this->txtValue->Text, result->Type, valueRaw)) {
				MessageBox::Show(this,
					L"Giá trị không hợp lệ. Hãy nhập số đúng kiểu của địa chỉ.",
					L"Lỗi", MessageBoxButtons::OK, MessageBoxIcon::Warning);
				return;
			}

			// Gọi Bridge → Native → WriteProcessMemory
			bool ok = ScannerBridge::WriteValue(
				_processHandle, result->Address, result->Type, valueRaw);

			if (ok) {
				// Update local result để DGV hiển thị giá trị mới ngay
				result->CurrentRaw = valueRaw;
				this->dgvResults->Refresh();  // redraw cell
				this->lblStatus->Text = String::Format(
					L"Đã ghi {0} vào địa chỉ 0x{1:X16}",
					this->txtValue->Text, result->Address);
				this->lblStatus->ForeColor = DarkTheme::Success;
			} else {
				this->lblStatus->Text = String::Format(
					L"Ghi thất bại tại địa chỉ 0x{0:X16} (memory protect?)",
					result->Address);
				this->lblStatus->ForeColor = DarkTheme::Danger;
			}
		}

		// =================================================================
		//  OnRefreshTick — auto-refresh giá trị các địa chỉ trong DGV
		// =================================================================
		// Tick mỗi 500ms. Đọc lại giá trị tại tất cả địa chỉ trong _results,
		// cập nhật CurrentRaw, force DGV redraw.
		// → Khi target process đổi value, DGV thấy live (delay tối đa 0.5s).
		//
		// Performance note: nếu _results có 100k+ entries, refresh sẽ tốn
		// thời gian. Limit ở 5000 row để tránh lag UI.
		void OnRefreshTick(Object^ sender, EventArgs^ e) {
			if (_processHandle == IntPtr::Zero) return;
			if (_results == nullptr || _results->Count == 0) return;
			if (_results->Count > 5000) return;  // skip nếu danh sách quá lớn

			// Update giá trị in-place
			ScannerBridge::RefreshValues(_processHandle, _results);

			// Force DGV redraw để show CurrentDisplay mới
			// (List<T> không tự notify changes → ta gọi Refresh thủ công)
			this->dgvResults->Refresh();
		}

		// =================================================================
		//  OnFreezeTick — ghi giá trị frozen liên tục
		// =================================================================
		// Tick mỗi 50ms. Duyệt _results, với mỗi row có IsFrozen=true thì
		// gọi WriteValue để ghi FrozenRaw vào address.
		//
		// → Process target cố đổi giá trị thì lập tức bị ghi đè lại sau 50ms.
		//   Hiệu ứng: giá trị "khóa cứng" — đây là ma thuật của Cheat Engine.
		void OnFreezeTick(Object^ sender, EventArgs^ e) {
			if (_processHandle == IntPtr::Zero) return;
			if (_results == nullptr) return;

			for each (ManagedScanResult^ r in _results) {
				if (r->IsFrozen) {
					ScannerBridge::WriteValue(
						_processHandle, r->Address, r->Type, r->FrozenRaw);
				}
			}
		}

		// =================================================================
		//  OnCellDirtyStateChanged — commit checkbox edit ngay khi click
		// =================================================================
		// DGV mặc định chỉ commit edit khi user click sang row khác. Với
		// checkbox, user muốn click 1 phát là toggle xong → commit ngay
		// để CellValueChanged fire ngay.
		void OnCellDirtyStateChanged(Object^ sender, EventArgs^ e) {
			if (this->dgvResults->IsCurrentCellDirty) {
				this->dgvResults->CommitEdit(
					DataGridViewDataErrorContexts::Commit);
			}
		}

		// =================================================================
		//  OnCellValueChanged — handle freeze checkbox toggle
		// =================================================================
		// Khi user tick/untick checkbox cột Frozen:
		//   - Tick (IsFrozen turn true): copy CurrentRaw → FrozenRaw
		//     (giá trị tại thời điểm freeze sẽ được lock)
		//   - Untick: không cần làm gì (timerFreeze sẽ skip row này)
		// =================================================================
		//  OnSaveSessionClick — lưu danh sách kết quả ra file .cvn
		// =================================================================
		// Format text đơn giản, mỗi dòng 1 địa chỉ:
		//   ADDRESS_HEX TYPE FROZEN FROZEN_RAW
		// VD: 0x2063ACB0000 Int32 frozen 101
		// File mở Notepad đọc được, có thể sửa tay.
		void OnSaveSessionClick(Object^ sender, EventArgs^ e) {
			if (_results == nullptr || _results->Count == 0) {
				MessageBox::Show(this,
					L"Chưa có kết quả nào để lưu. Hãy quét trước.",
					L"Không có dữ liệu",
					MessageBoxButtons::OK, MessageBoxIcon::Information);
				return;
			}

			auto sfd = gcnew SaveFileDialog();
			sfd->Filter = L"CheatVN Session (*.cvn)|*.cvn|Tất cả file (*.*)|*.*";
			sfd->FileName = L"session.cvn";
			sfd->Title = L"Lưu CheatVN session";
			if (sfd->ShowDialog(this) != System::Windows::Forms::DialogResult::OK)
				return;

			auto sb = gcnew System::Text::StringBuilder();
			sb->AppendLine(L"# CheatVN Scan Session");
			sb->AppendLine(String::Format(L"# Process: {0}",
				_currentProcess != nullptr ? _currentProcess->Name : L"unknown"));
			sb->AppendLine(String::Format(L"# Saved: {0}",
				DateTime::Now.ToString(L"yyyy-MM-dd HH:mm:ss")));
			sb->AppendLine(L"# Format: ADDRESS TYPE FROZEN FROZEN_RAW");

			for each (ManagedScanResult^ r in _results) {
				sb->AppendLine(String::Format(
					L"0x{0:X16} {1} {2} {3}",
					r->Address,
					r->Type.ToString(),
					r->IsFrozen ? L"frozen" : L"free",
					r->FrozenRaw));
			}

			try {
				System::IO::File::WriteAllText(
					sfd->FileName, sb->ToString(), System::Text::Encoding::UTF8);
				this->lblStatus->Text = String::Format(
					L"Đã lưu {0} địa chỉ vào {1}",
					_results->Count,
					System::IO::Path::GetFileName(sfd->FileName));
				this->lblStatus->ForeColor = DarkTheme::Success;
			}
			catch (Exception^ ex) {
				MessageBox::Show(this,
					String::Format(L"Lỗi khi ghi file:\n{0}", ex->Message),
					L"Lỗi", MessageBoxButtons::OK, MessageBoxIcon::Error);
			}
		}

		// =================================================================
		//  OnLoadSessionClick — đọc session từ file .cvn, bind lên DGV
		// =================================================================
		// Lưu ý: chỉ load address + type + frozen state. CurrentRaw sẽ
		// được auto-refresh điền sau khi attach process.
		void OnLoadSessionClick(Object^ sender, EventArgs^ e) {
			auto ofd = gcnew OpenFileDialog();
			ofd->Filter = L"CheatVN Session (*.cvn)|*.cvn|Tất cả file (*.*)|*.*";
			ofd->Title = L"Mở CheatVN session";
			if (ofd->ShowDialog(this) != System::Windows::Forms::DialogResult::OK)
				return;

			auto loaded = gcnew List<ManagedScanResult^>();
			try {
				auto lines = System::IO::File::ReadAllLines(
					ofd->FileName, System::Text::Encoding::UTF8);

				for each (String^ line in lines) {
					// Skip dòng comment và dòng trống
					if (String::IsNullOrWhiteSpace(line)) continue;
					if (line->StartsWith(L"#")) continue;

					// Parse: ADDRESS TYPE FROZEN FROZEN_RAW
					auto parts = line->Split(
						gcnew cli::array<wchar_t>{ L' ', L'\t' },
						StringSplitOptions::RemoveEmptyEntries);
					if (parts->Length < 4) continue;

					try {
						auto r = gcnew ManagedScanResult();
						// Address: "0x..." → strip 0x → parse hex
						String^ addrStr = parts[0];
						if (addrStr->StartsWith(L"0x") || addrStr->StartsWith(L"0X"))
							addrStr = addrStr->Substring(2);
						r->Address = Convert::ToUInt64(addrStr, 16);

						// Type: parse enum name
						r->Type = (ManagedValueType)Enum::Parse(
							ManagedValueType::typeid, parts[1]);

						// Frozen state
						r->IsFrozen = (parts[2]->Equals(L"frozen",
							StringComparison::OrdinalIgnoreCase));
						r->FrozenRaw = Int64::Parse(parts[3]);

						r->CurrentRaw = r->FrozenRaw;  // tạm hiển thị
						r->HasPrevious = false;

						loaded->Add(r);
					}
					catch (Exception^) {
						// Skip dòng parse lỗi
					}
				}
			}
			catch (Exception^ ex) {
				MessageBox::Show(this,
					String::Format(L"Lỗi đọc file:\n{0}", ex->Message),
					L"Lỗi", MessageBoxButtons::OK, MessageBoxIcon::Error);
				return;
			}

			_results = loaded;
			this->dgvResults->DataSource = nullptr;
			this->dgvResults->DataSource = _results;
			this->lblResultCount->Text = String::Format(
				L"Đã load: {0} địa chỉ", _results->Count);
			this->lblStatus->Text = String::Format(
				L"Đã load session từ {0}. Hãy attach process để xem giá trị live.",
				System::IO::Path::GetFileName(ofd->FileName));
			this->lblStatus->ForeColor = DarkTheme::Accent;
			UpdateScanUIState();

			// Nếu đã attach process → refresh ngay để hiện giá trị thật
			if (_processHandle != IntPtr::Zero) {
				ScannerBridge::RefreshValues(_processHandle, _results);
				this->dgvResults->Refresh();
			}
		}

		void OnCellValueChanged(Object^ sender, DataGridViewCellEventArgs^ e) {
			if (e->RowIndex < 0) return;
			// Chỉ care về cột Frozen
			if (this->dgvResults->Columns[e->ColumnIndex] != this->colFrozen) return;

			auto row = this->dgvResults->Rows[e->RowIndex];
			if (row->DataBoundItem == nullptr) return;

			auto result = safe_cast<ManagedScanResult^>(row->DataBoundItem);
			if (result == nullptr) return;

			if (result->IsFrozen) {
				// Vừa freeze → snapshot giá trị hiện tại làm "frozen value"
				result->FrozenRaw = result->CurrentRaw;
				this->lblStatus->Text = String::Format(
					L"Đã đóng băng địa chỉ 0x{0:X16} ở giá trị {1}",
					result->Address, result->CurrentDisplay);
				this->lblStatus->ForeColor = DarkTheme::Accent;
			} else {
				this->lblStatus->Text = String::Format(
					L"Đã mở băng địa chỉ 0x{0:X16}", result->Address);
				this->lblStatus->ForeColor = DarkTheme::TextPrimary;
			}
		}
	};
}

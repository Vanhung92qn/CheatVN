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
    using namespace System::Text;

    /// <summary>
    /// BenchmarkForm — đo hiệu năng scanner với nhiều cấu hình:
    ///   1. Scalar single-thread (baseline)
    ///   2. Scalar multi-thread
    ///   3. SIMD AVX2 single-thread
    ///   4. SIMD AVX2 multi-thread (default)
    /// Hiển thị bảng so sánh thời gian + speedup. Dùng để demo cho thầy.
    /// </summary>
    public ref class BenchmarkForm : public System::Windows::Forms::Form
    {
    public:
        BenchmarkForm(IntPtr hProcess)
        {
            _hProcess = hProcess;
            InitializeComponent();
            DarkTheme::Apply(this);
            this->txtOutput->BackColor = DarkTheme::WindowSurface;
            this->txtOutput->ForeColor = DarkTheme::TextPrimary;
            this->txtOutput->Font = gcnew System::Drawing::Font(L"Consolas", 10.0f);

            this->btnRun->Click += gcnew EventHandler(this, &BenchmarkForm::OnRunClick);
            this->btnClose->Click += gcnew EventHandler(this, &BenchmarkForm::OnCloseClick);
        }

    protected:
        ~BenchmarkForm() { if (components) delete components; }

    private:
        IntPtr _hProcess;
        System::ComponentModel::IContainer^ components;
        System::Windows::Forms::Panel^ pnlTop;
        System::Windows::Forms::Label^ lblValue;
        System::Windows::Forms::TextBox^ txtValue;
        System::Windows::Forms::Label^ lblInfo;
        System::Windows::Forms::Button^ btnRun;
        System::Windows::Forms::TextBox^ txtOutput;
        System::Windows::Forms::Panel^ pnlBottom;
        System::Windows::Forms::Button^ btnClose;

#pragma region Windows Form Designer generated code
        void InitializeComponent(void)
        {
            this->components = gcnew System::ComponentModel::Container();
            this->pnlTop = gcnew System::Windows::Forms::Panel();
            this->lblValue = gcnew System::Windows::Forms::Label();
            this->txtValue = gcnew System::Windows::Forms::TextBox();
            this->lblInfo = gcnew System::Windows::Forms::Label();
            this->btnRun = gcnew System::Windows::Forms::Button();
            this->txtOutput = gcnew System::Windows::Forms::TextBox();
            this->pnlBottom = gcnew System::Windows::Forms::Panel();
            this->btnClose = gcnew System::Windows::Forms::Button();

            this->pnlTop->SuspendLayout();
            this->pnlBottom->SuspendLayout();
            this->SuspendLayout();

            this->pnlTop->Controls->Add(this->btnRun);
            this->pnlTop->Controls->Add(this->lblInfo);
            this->pnlTop->Controls->Add(this->txtValue);
            this->pnlTop->Controls->Add(this->lblValue);
            this->pnlTop->Dock = System::Windows::Forms::DockStyle::Top;
            this->pnlTop->Size = System::Drawing::Size(800, 76);

            this->lblValue->AutoSize = true;
            this->lblValue->Location = System::Drawing::Point(12, 14);
            this->lblValue->Text = L"Giá trị Int32 cần quét (chọn số lạ để ít kết quả):";

            this->txtValue->Location = System::Drawing::Point(12, 36);
            this->txtValue->Size = System::Drawing::Size(160, 23);
            this->txtValue->Font = gcnew System::Drawing::Font(L"Consolas", 9.0f);
            this->txtValue->Text = L"12345";

            this->lblInfo->Location = System::Drawing::Point(192, 38);
            this->lblInfo->Size = System::Drawing::Size(450, 22);
            this->lblInfo->Text = L"Chạy 4 cấu hình: scalar 1T, scalar NT, SIMD 1T, SIMD NT. Hiển thị speedup.";

            this->btnRun->Location = System::Drawing::Point(660, 32);
            this->btnRun->Size = System::Drawing::Size(130, 30);
            this->btnRun->Text = L"Chạy benchmark";
            this->btnRun->BackColor = DarkTheme::Accent;
            this->btnRun->ForeColor = Color::White;
            this->btnRun->FlatStyle = System::Windows::Forms::FlatStyle::Flat;

            this->txtOutput->Dock = System::Windows::Forms::DockStyle::Fill;
            this->txtOutput->Multiline = true;
            this->txtOutput->ReadOnly = true;
            this->txtOutput->ScrollBars = System::Windows::Forms::ScrollBars::Vertical;
            this->txtOutput->Text = L"Bấm 'Chạy benchmark' để bắt đầu...";

            this->pnlBottom->Controls->Add(this->btnClose);
            this->pnlBottom->Dock = System::Windows::Forms::DockStyle::Bottom;
            this->pnlBottom->Size = System::Drawing::Size(800, 44);

            this->btnClose->Location = System::Drawing::Point(700, 8);
            this->btnClose->Size = System::Drawing::Size(90, 28);
            this->btnClose->Text = L"Đóng";
            this->btnClose->Anchor = static_cast<System::Windows::Forms::AnchorStyles>(
                System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Right);

            this->AutoScaleDimensions = System::Drawing::SizeF(7, 15);
            this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
            this->ClientSize = System::Drawing::Size(800, 500);
            this->Controls->Add(this->txtOutput);
            this->Controls->Add(this->pnlTop);
            this->Controls->Add(this->pnlBottom);
            this->Name = L"BenchmarkForm";
            this->Text = L"Benchmark — So sánh tốc độ scanner";
            this->StartPosition = System::Windows::Forms::FormStartPosition::CenterParent;

            this->pnlTop->ResumeLayout(false);
            this->pnlTop->PerformLayout();
            this->pnlBottom->ResumeLayout(false);
            this->ResumeLayout(false);
        }
#pragma endregion

        void OnCloseClick(Object^ sender, EventArgs^ e) {
            this->Close();
        }

        // =================================================================
        //  OnRunClick — chạy 4 benchmark, in kết quả lên textbox
        // =================================================================
        void OnRunClick(Object^ sender, EventArgs^ e) {
            // Parse target value
            Int64 targetRaw;
            if (!ScannerBridge::TryParseValue(
                    this->txtValue->Text, ManagedValueType::Int32, targetRaw)) {
                MessageBox::Show(this, L"Giá trị không hợp lệ", L"Lỗi",
                    MessageBoxButtons::OK, MessageBoxIcon::Warning);
                return;
            }

            this->btnRun->Enabled = false;
            this->Cursor = Cursors::WaitCursor;
            this->txtOutput->Text = L"";
            Application::DoEvents();

            auto sb = gcnew StringBuilder();
            sb->AppendLine(L"╔══════════════════════════════════════════════════════════╗");
            sb->AppendLine(L"║          CheatVN — Benchmark Scanner Performance         ║");
            sb->AppendLine(L"╚══════════════════════════════════════════════════════════╝");
            sb->AppendLine();
            sb->AppendLine(String::Format(L"Target value:  Int32 = {0}", this->txtValue->Text));
            sb->AppendLine(String::Format(L"CPU cores:     {0}", Environment::ProcessorCount));
            sb->AppendLine();
            sb->AppendLine(L"Cấu hình                          Time (s)    Speedup    Results");
            sb->AppendLine(L"────────────────────────────────  ─────────   ────────   ───────");
            this->txtOutput->Text = sb->ToString();
            Application::DoEvents();

            // ─── Run 4 configs ────────────────────────────────────
            // 1. Scalar single-thread (baseline)
            double t1 = RunOne(targetRaw, 1, false, sb);
            // 2. Scalar multi-thread (auto)
            double t2 = RunOne(targetRaw, 0, false, sb);
            // 3. SIMD single-thread
            double t3 = RunOne(targetRaw, 1, true, sb);
            // 4. SIMD multi-thread (auto, default config of CheatVN)
            double t4 = RunOne(targetRaw, 0, true, sb);

            // ─── Summary ──────────────────────────────────────────
            sb->AppendLine();
            sb->AppendLine(L"─────────────────────── TÓM TẮT ─────────────────────────");
            sb->AppendLine(String::Format(
                L"Multi-thread speedup (so với scalar 1T):  {0:F2}x", t1 / t2));
            sb->AppendLine(String::Format(
                L"SIMD speedup (so với scalar 1T):          {0:F2}x", t1 / t3));
            sb->AppendLine(String::Format(
                L"Multi-thread + SIMD speedup (final):      {0:F2}x", t1 / t4));
            sb->AppendLine();
            sb->AppendLine(L"=> Đây là số liệu để demo trong báo cáo đồ án.");
            sb->AppendLine(String::Format(
                L"   Single-thread scalar: {0:F3}s  →  Multi-thread + SIMD: {1:F3}s",
                t1, t4));

            this->txtOutput->Text = sb->ToString();
            this->btnRun->Enabled = true;
            this->Cursor = Cursors::Default;
        }

        double RunOne(Int64 targetRaw, unsigned int nThreads, bool simd, StringBuilder^ sb) {
            String^ label = String::Format(
                L"{0}, {1} thread{2}",
                simd ? L"SIMD AVX2" : L"Scalar",
                nThreads == 0 ? (unsigned int)Environment::ProcessorCount : nThreads,
                nThreads == 1 ? L"" : L"s");

            auto sw = Stopwatch::StartNew();
            int count = ScannerBridge::FirstScanBenchmark(
                _hProcess, ManagedValueType::Int32,
                targetRaw, ManagedScanOperator::Exact, nThreads, simd);
            sw->Stop();
            double sec = sw->Elapsed.TotalSeconds;

            sb->AppendLine(String::Format(
                L"{0,-32}  {1,8:F3}    {2,7}   {3,8}",
                label, sec, L"--", count));

            // Update UI ngay sau mỗi config
            this->txtOutput->Text = sb->ToString();
            Application::DoEvents();
            return sec;
        }
    };

} // namespace GUI

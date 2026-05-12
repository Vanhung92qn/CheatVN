#include "MainForm.h"
#include "ProcessManagerBridge.h"

using namespace System;
using namespace System::Windows::Forms;

// =====================================================================
//  Program.cpp — Entry point của CheatVN
//
//  [STAThreadAttribute] đánh dấu main thread là Single-Threaded Apartment
//  — bắt buộc cho WinForms vì WinForms dùng COM (drag-drop, clipboard).
// =====================================================================
[STAThreadAttribute]
int main(array<System::String^>^ args)
{
    // ─── Bật SeDebugPrivilege khi khởi động ─────────────────────────
    // Đây là privilege cho phép tool mở handle vào process system / khác user.
    // Nếu user không chạy as Administrator → fail, nhưng app vẫn chạy được
    // — chỉ là không truy cập được process bảo vệ.
    GUI::ProcessManagerBridge::EnableDebugPrivilege();

    // ─── Bật visual styles (Win XP+ theme: gradient, scrollbar đẹp) ─
    Application::EnableVisualStyles();
    Application::SetCompatibleTextRenderingDefault(false);

    // ─── Tạo MainForm và chạy message loop ─────────────────────────
    // Application::Run block tới khi MainForm đóng.
    Application::Run(gcnew GUI::MainForm());

    return 0;
}

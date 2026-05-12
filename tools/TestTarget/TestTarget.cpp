// =====================================================================
//  TestTarget.cpp — App test cho CheatVN.
//
//  Mục đích: demo CheatVN scan + filter + edit value.
//
//  Cơ chế:
//   - Cấp phát 1 int trên 1 page memory RIÊNG (qua VirtualAlloc) →
//     địa chỉ cố định, không bị GC dọn, dễ scan tìm thấy.
//   - In ra địa chỉ + giá trị hiện tại để user xác minh CheatVN scan
//     đúng địa chỉ này.
//   - User dùng phím +/- để đổi giá trị → CheatVN dùng "Quét lại" để
//     lọc, kiểm tra workflow.
//
//  Build: chạy build.bat trong "Developer Command Prompt for VS 2022".
// =====================================================================

#include <Windows.h>
#include <conio.h>
#include <cstdio>
#include <cstdlib>

int main() {
    // Bật UTF-8 output để in tiếng Việt + emoji nếu cần
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleTitleW(L"TestTarget — App test cho CheatVN");

    // Cấp phát 1 int trên page memory riêng (4KB).
    // VirtualAlloc cho 1 page sạch ở 1 địa chỉ cố định, dễ tìm bằng scanner.
    // Không dùng `new int` vì heap allocator có thể đặt cạnh data khác → noise.
    int* value = static_cast<int*>(
        VirtualAlloc(nullptr, sizeof(int), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE)
    );
    if (!value) {
        printf("VirtualAlloc failed!\n");
        return 1;
    }

    *value = 100;  // giá trị khởi đầu

    while (true) {
        system("cls");
        printf("===============================================\n");
        printf("   TestTarget - app demo cho CheatVN\n");
        printf("===============================================\n\n");
        printf("  Process ID:     %lu\n", GetCurrentProcessId());
        printf("  Dia chi value:  0x%016llX\n", (unsigned long long)value);
        printf("  Gia tri hien:   %d\n\n", *value);
        printf("-----------------------------------------------\n");
        printf(" Cach dung CheatVN:\n");
        printf("  1. CheatVN -> Chon tien trinh -> TestTarget.exe\n");
        printf("  2. Quet Int32 = %d (gia tri hien)\n", *value);
        printf("  3. Bam +/- de doi gia tri ben duoi\n");
        printf("  4. CheatVN -> Quet lai voi gia tri moi\n");
        printf("  5. Lap lai cho den khi con 1 dia chi\n");
        printf("  6. Dia chi do PHAI = 0x%llX (in tren cung)\n",
               (unsigned long long)value);
        printf("-----------------------------------------------\n\n");
        printf(" Phim:\n");
        printf("  + : tang 1\n");
        printf("  - : giam 1\n");
        printf("  * : tang 100\n");
        printf("  / : giam 100\n");
        printf("  s : set gia tri tuy y\n");
        printf("  q : thoat\n\n");
        printf("Bam phim: ");
        fflush(stdout);

        int c = _getch();
        switch (c) {
            case '+': (*value)++; break;
            case '-': (*value)--; break;
            case '*': *value += 100; break;
            case '/': *value -= 100; break;
            case 's': {
                printf("\nNhap gia tri moi (so nguyen): ");
                int v;
                if (scanf_s("%d", &v) == 1) {
                    *value = v;
                }
                while (getchar() != '\n');  // flush stdin
                break;
            }
            case 'q':
            case 'Q':
                VirtualFree(value, 0, MEM_RELEASE);
                return 0;
        }
    }
}

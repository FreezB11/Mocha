// // /// @file: main.c
// // #include <stdio.h>
// // #include <stddef.h>
// // #include <wchar.h>
// // #include <locale.h>
// // #include <stdlib.h>

// #define font(r,g,b) "\x1b[38;2;" #r ";" #g ";" #b "m"
// #define bg(r,g,b)   "\x1b[48;2;" #r ";" #g ";" #b "m"

// // // we make a matrix coz yea y not

// // size_t rows = 10, cols = 20;

// // wchar_t **grid = malloc(rows * sizeof *grid);
// // for (size_t i = 0; i < rows; ++i) {
// //     grid[i] = malloc((cols + 1) * sizeof *grid[i]);
// //     grid[i][cols] = L'\0';
// // }

// // int main(){
// //     setlocale(LC_ALL, "");
// //     wchar_t test[] = L"▀▀";
// //     wchar_t alpha[] = L"▙▟";
// //     wchar_t another[4] = {0};
// //     another[0] = L'▞';
// //     wprintf(L"%s",font(255, 50, 50));
// //     wprintf(L"%s",bg(255, 255, 50));
// //     wprintf(L"%ls\n", test);
// //     wprintf(L"%ls\n", alpha);
// //     wprintf(L"%ls\n", another);

// //     return 0;
// // }
// #include <stdio.h>
// #include <stdlib.h>
// #include <wchar.h>
// #include <locale.h>

// int main(void) {
//     setlocale(LC_ALL, "");

//     size_t rows = 5, cols = 5;

//     wchar_t **grid = malloc(rows * sizeof *grid);
//     if (!grid) return 1;

//     for (size_t i = 0; i < rows; ++i) {
//         grid[i] = malloc((cols + 1) * sizeof *grid[i]);
//         if (!grid[i]) return 1;
//         grid[i][cols] = L'\0';
//     }

//     grid[0][0] = L'▀';
//     wprintf(L"%lc\n", grid[0][0]);

//     /* cleanup */
//     for (size_t i = 0; i < rows; ++i)
//         free(grid[i]);
//     free(grid);

//     return 0;
// }
#include <stdio.h>

#define font(r,g,b) "\x1b[38;2;" #r ";" #g ";" #b "m"
#define bg(r,g,b)   "\x1b[48;2;" #r ";" #g ";" #b "m"
#define reset       "\x1b[0m"

int main(void) {
    int rows = 8;
    int cols = 8;

    // We'll use full block for the chessboard cells
    const char *block = "▀";

    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            // if ((r + c) % 2 == 0) {
                // White-ish square
                printf("%s%s%s", font(255,255,255), bg(0,0,0), block);
                // printf("%s%s%s", bg(0,0,0), font(255,255,255), block);
            // } else {
                // Black-ish square
                // printf("%s%s%s", font(0,0,0), bg(255,255,255), block);
                // printf("%s%s%s", bg(255,255,255), font(0,0,0), block);
            // }
        }
        printf("%s\n", reset); // reset colors at end of row
    }

    printf("%s", reset); // final reset
    return 0;
}

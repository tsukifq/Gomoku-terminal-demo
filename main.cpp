#include "include/GameEngine.h"
#ifdef _WIN32
#include <windows.h>
#endif

int main() {
#ifdef _WIN32
    // Enable ANSI escape codes for Windows 10+
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(hOut, &dwMode);
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, dwMode);
    // Set output code page to UTF-8 (optional, but good for symbols)
    SetConsoleOutputCP(CP_UTF8);
#endif

    GameEngine engine;
    engine.run();
    return 0;
}

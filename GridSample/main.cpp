
#include "stdafx.h"
#include "GridSample.h"

int main(int argc, char* argv[])
{
    InitProcessDpiAwareness();

    Window wnd1(10, 8, 40);

    MSG msg;
    DbgPrint("Entering Message Loop.\n\n");
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    DbgPrint("Thread Exiting...\n");
    return 0;
}


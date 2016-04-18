
#include "stdafx.h"
#include "GridSample.h"

int main(int argc, char* argv[])
{
    InitProcessDpiAwareness();

    DbgPrint("Creating window...\n");
    Window wnd1(15, // cx
                15, // cy
                50, // blocksize 
                {
                    TRUE,  //Resize;
                    FALSE,  //Snap Window To Grid;
                    TRUE, //Restrict To Monitor Size;
                    FALSE  //Always Entirely On Monitor
                });

    MSG msg;
    DbgPrint("Entering Message Loop.\n\n");
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    DbgPrint("Thread Exiting...\n");
    return 0;
}


#pragma once
#include <windows.h>
#include <shellscalingapi.h>

#define RECTWIDTH(rc) (rc.right - rc.left)
#define RECTHEIGHT(rc) (rc.bottom - rc.top)

//
// Grid related functions
//
VOID InitGrid(HWND);
VOID DrawGrid(HWND hwnd, HDC hdc);
VOID SetGridDpi(UINT DPI);
VOID GetGridSize(UINT &cx, UINT &cy);
BOOL SizeGridToWindow(RECT rcClient);

//
// DPI related functions
//

#define ScaleToPhys(DPI, val)   MulDiv(val, DPI, 96)
#define ScaleToLog(DPI, val)    MulDiv(val, 96, DPI)
#define DPIinPercentage(DPI)    ScaleToPhys(DPI, 100)
#define DPItoFloat(DPI)         ((float)DPI / 96)

extern PROCESS_DPI_AWARENESS gpda;
#define IsProcessPerMonitorDpiAware() (gpda == PROCESS_PER_MONITOR_DPI_AWARE)
#define IsProcessSystemDpiAware()     (gpda == PROCESS_SYSTEM_DPI_AWARE)

extern BOOL bHandlingDpiChange;
BOOL InitProcessDpiAwareness();
BOOL EnableNonClientScalingForWindow(HWND hwnd);
BOOL AdjustWindowRectExForDpi_l(LPRECT, DWORD, DWORD, BOOL, UINT DPI);
UINT GetDpiForWindow_l(HWND hwnd);

//
// DbgPrint / DbgPrintError (and fancy colors for console text)
// 

#define FOREGROUND_WHITE		    (FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_GREEN)
#define FOREGROUND_YELLOW       	(FOREGROUND_RED | FOREGROUND_GREEN)
#define FOREGROUND_CYAN		        (FOREGROUND_BLUE | FOREGROUND_GREEN)
#define FOREGROUND_MAGENTA	        (FOREGROUND_RED | FOREGROUND_BLUE)
#define FOREGROUND_BLACK		    0
#define FOREGROUND_INTENSE_RED		(FOREGROUND_RED | FOREGROUND_INTENSITY)
#define FOREGROUND_INTENSE_GREEN	(FOREGROUND_GREEN | FOREGROUND_INTENSITY)
#define FOREGROUND_INTENSE_BLUE		(FOREGROUND_BLUE | FOREGROUND_INTENSITY)
#define FOREGROUND_INTENSE_WHITE	(FOREGROUND_WHITE | FOREGROUND_INTENSITY)
#define FOREGROUND_INTENSE_YELLOW	(FOREGROUND_YELLOW | FOREGROUND_INTENSITY)
#define FOREGROUND_INTENSE_CYAN		(FOREGROUND_CYAN | FOREGROUND_INTENSITY)
#define FOREGROUND_INTENSE_MAGENTA	(FOREGROUND_MAGENTA | FOREGROUND_INTENSITY)
#define BACKGROUND_WHITE		    (BACKGROUND_RED | BACKGROUND_BLUE | BACKGROUND_GREEN)
#define BACKGROUND_YELLOW	        (BACKGROUND_RED | BACKGROUND_GREEN)
#define BACKGROUND_CYAN		        (BACKGROUND_BLUE | BACKGROUND_GREEN)
#define BACKGROUND_MAGENTA	        (BACKGROUND_RED | BACKGROUND_BLUE)
#define BACKGROUND_BLACK		    0
#define BACKGROUND_INTENSE_RED		(BACKGROUND_RED | BACKGROUND_INTENSITY)
#define BACKGROUND_INTENSE_GREEN	(BACKGROUND_GREEN | BACKGROUND_INTENSITY)
#define BACKGROUND_INTENSE_BLUE		(BACKGROUND_BLUE | BACKGROUND_INTENSITY)
#define BACKGROUND_INTENSE_WHITE	(BACKGROUND_WHITE | BACKGROUND_INTENSITY)
#define BACKGROUND_INTENSE_YELLOW	(BACKGROUND_YELLOW | BACKGROUND_INTENSITY)
#define BACKGROUND_INTENSE_CYAN		(BACKGROUND_CYAN | BACKGROUND_INTENSITY)
#define BACKGROUND_INTENSE_MAGENTA	(BACKGROUND_MAGENTA | BACKGROUND_INTENSITY)

extern WORD gPrevConsoleTextAttribs;
// note: this hacky mess only works while all prints happen on the same thread

__inline void SetConsoleColor(WORD Color) {
    CONSOLE_SCREEN_BUFFER_INFO Info;
    HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
    GetConsoleScreenBufferInfo(hStdout, &Info);
    gPrevConsoleTextAttribs = Info.wAttributes;
    SetConsoleTextAttribute(hStdout, Color);
}

__inline void ResetConsoleColor() {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), gPrevConsoleTextAttribs);
}

#define ErrorTextApptribs   BACKGROUND_RED/*| FOREGROUND_RED */| BACKGROUND_INTENSITY
#define DbgPrintError(...)  SetConsoleColor(ErrorTextApptribs); \
                            printf (__VA_ARGS__); \
                            ResetConsoleColor()

#define RegTextApptribs     FOREGROUND_WHITE | FOREGROUND_INTENSITY
#define DbgPrint(...)       SetConsoleColor(RegTextApptribs); \
                            printf (__VA_ARGS__); \
                            ResetConsoleColor()

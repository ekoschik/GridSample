#pragma once

#include <windows.h>
#include <shellscalingapi.h>
#include <Windowsx.h>
#include <map>

class Grid {
public:
    VOID Init(INT cx, INT cy, INT blocksize);
    VOID SetDpi(UINT DPI);

    VOID GetSize(UINT &cx, UINT &cy);
    BOOL SizeToWindow(HWND hwnd);
    VOID Draw(HDC hdc);

    // Adjust the grid for scroll/ ctrl+scroll
    VOID AdjustBlockSize(INT delta);
    VOID AdjustGridSize(INT delta);


private:
    UINT DPI;

    int GetBlockSizeForDpi(UINT dpi);

    int blockSizeModifier = 7;
    int baseBlockSize;
    int adjustedBlockSize;

    int grid_cx, grid_cy;

    HBRUSH hbrGrid1, hbrGrid2;
    COLORREF rgbGrid1 = RGB(0, 51, 204); // blue
    COLORREF rgbGrid2 = RGB(204, 153, 0); // dark yellow
};

class MWdelta {
    INT _delta;
public:
    MWdelta() { _delta = 0; }
    INT inc(INT delta) {
        _delta += delta;
        if (_delta <= -120 || _delta >= 120) {
            BOOL bUp = _delta > 0;
            _delta -= (bUp ? 120 : -120);
            return (bUp ? 1 : -1);
        }
        return 0;
    }
};

class Window {
public:
    // Constructors
    Window();
    Window(INT cx, INT cy, INT blocksize);
    Window(INT cx,
           INT cy,
           INT blocksize,
           BOOL bResize,
           BOOL bSnapWindowToGrid,
           BOOL bRestrictToMonitorSize,
           BOOL bAlwaysEntirelyOnMonitor);

    // Handling window messages
    VOID Draw(HDC hdc);
    VOID HandleDpiChange(UINT DPI, RECT* prc);
    VOID HandleWindowPosChanging(PWINDOWPOS pwp);
    VOID HandleWindowPosChanged();
    VOID HandleEnterExitMoveSize(BOOL bEnter);
    VOID HandleMouseWheel(HWND hwnd, WPARAM wParam, LPARAM lParam);
    VOID HandleDoubleClick(POINT pt);

    // Helper routines
    BOOL GetWindowSizeForCurrentGridSize(UINT &cx, UINT &cy);
    VOID ResizeSuggestionRectForDpiChange(PRECT prcSuggestion);
    BOOL EnforceWindowPosRestrictions(PRECT prcWindow);
    VOID SizeWindowToGrid(PPOINT pptResizeAround);
    
private:

    HWND hwnd;
    UINT DPI;

    // The window class only needs to be registered one time
    static BOOL EnsureWindowIsRegistered(HINSTANCE, LPWSTR);

    // Private Initializers, and WM_CREATE handler
    VOID Init(INT cx, INT cy, INT blocksize,
        BOOL bResize,
        BOOL bSnapWindowToGrid,
        BOOL bRestrictToMonitorSize,
        BOOL bAlwaysEntirelyOnMonitor);
    VOID Window::Init(INT cx, INT cy, INT blocksize);
    VOID Create(HWND _hwnd);

    // Per Window Settings
    struct Settings {
        BOOL bResize;
        BOOL bSnapWindowToGrid;
        BOOL bRestrictToMonitorSize;
        BOOL bAlwaysEntirelyOnMonitor;
    };
    Settings settings;

    // Each window has it's own Grid
    Grid grid;

    // Each window tracks if it's in the process of handling
    // WM_DPICHANGED, in order to error if a DPI change causes 
    // the grid to be resized.
    BOOL bHandlingDpiChange = FALSE;

    // In order to behave correctly while the window is
    // snapped to corners/ sides of monitors, keep track
    // of whether the window is 'snapped'.  TODO: SuperHacky
    BOOL bTrackSnap = FALSE;
    BOOL IsSnapped();

    // Delta accumulater for WM_MOUSEWHEEL
    MWdelta mw_delta;

    // Defaults
    const Settings DefSettings = {
        TRUE,  //Resize;
        FALSE, //Snap Window To Grid;
        TRUE,  //Restrict To Monitor Size;
        FALSE  //Always Entirely On Monitor
    };

    const INT DefGridCX = 15,
        DefGridCY = 15,
        DefGridBlockSize = 50;

    // Syntax Glue to allow a single WndProc to be used for several windows at a time
    static std::map<HWND, Window*> WindowMap;
    static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
    static Window* LookupWindow(HWND hwnd);

};

extern PROCESS_DPI_AWARENESS gpda;
#define IsProcessPerMonitorDpiAware() (gpda == PROCESS_PER_MONITOR_DPI_AWARE)
#define IsProcessSystemDpiAware()     (gpda == PROCESS_SYSTEM_DPI_AWARE)

BOOL InitProcessDpiAwareness();
UINT GetDpiForWindow(HWND hwnd);
BOOL EnableNonClientScalingForWindow(HWND hwnd);
BOOL GetWindowSizeForClientSize(HWND hwnd, UINT &cx, UINT &cy);

#define ScaleToPhys(DPI, val)   MulDiv(val, DPI, 96)
#define ScaleToLog(DPI, val)    MulDiv(val, 96, DPI)
#define DPIinPercentage(DPI)    ScaleToPhys(DPI, 100)
#define DPItoFloat(DPI)         ((float)DPI / 96)

#define RECTWIDTH(rc) (rc.right - rc.left)
#define RECTHEIGHT(rc) (rc.bottom - rc.top)
#define PRECTWIDTH(prc) (prc->right - prc->left)
#define PRECTHEIGHT(prc) (prc->bottom - prc->top)

__inline int EnforceMinimumValue(int val, int min) {
    return (val >= min) ? val : min;
}

__inline VOID ResizeRectAroundPoint(PRECT prc, UINT cx, UINT cy, POINT pt) {
    prc->left = pt.x - MulDiv(pt.x - prc->left, cx, PRECTWIDTH(prc));
    prc->top = pt.y - MulDiv(pt.y - prc->top, cy, PRECTHEIGHT(prc));
    prc->right = prc->left + cx;
    prc->bottom = prc->top + cy;
}

__inline BOOL KeepRectInsideBiggerRect(PRECT rcSmall, const PRECT rcBig) {
    int cx = PRECTWIDTH(rcSmall);
    int cy = PRECTHEIGHT(rcSmall);
    BOOL ret = FALSE;
    if (rcSmall->left < rcBig->left) {
        rcSmall->left = rcBig->left;
        rcSmall->right = rcSmall->left + cx;
        ret = TRUE;
    }
    if (rcSmall->top < rcBig->top) {
        rcSmall->top = rcBig->top;
        rcSmall->bottom = rcSmall->top + cy;
        ret = TRUE;
    }
    if (rcSmall->right > rcBig->right) {
        rcSmall->right = rcBig->right;
        rcSmall->left = rcSmall->right - cx;
        ret = TRUE;
    }
    if (rcSmall->bottom > rcBig->bottom) {
        rcSmall->bottom = rcBig->bottom;
        rcSmall->top = rcSmall->bottom - cy;
        ret = TRUE;
    }
    return ret;
}

//
// Dbg/ Error printing macros
//

__inline void SetConsoleColor(WORD Color, WORD &gPrevConsoleTextAttribs) {
    CONSOLE_SCREEN_BUFFER_INFO Info;
    HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
    GetConsoleScreenBufferInfo(hStdout, &Info);
    gPrevConsoleTextAttribs = Info.wAttributes;
    SetConsoleTextAttribute(hStdout, Color);
}

__inline void ResetConsoleColor(WORD gPrevConsoleTextAttribs) {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), gPrevConsoleTextAttribs);
}

#define INDENT "  ---> "

#define DbgPrintImpl(attr, ...) { \
                                  printf (__VA_ARGS__); \
                                }
                                //WORD gPrevConsoleTextAttribs; \
                                //SetConsoleColor(attr, gPrevConsoleTextAttribs); \
                                //ResetConsoleColor(gPrevConsoleTextAttribs); \
                                //}

#define DbgPrintError(...)  DbgPrintImpl(BACKGROUND_RED/*| FOREGROUND_RED */| BACKGROUND_INTENSITY, __VA_ARGS__)

#define FOREGROUND_WHITE    (FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_GREEN)
#define DbgPrint(...)       DbgPrintImpl(FOREGROUND_WHITE | FOREGROUND_INTENSITY, __VA_ARGS__)

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

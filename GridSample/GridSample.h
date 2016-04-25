#pragma once

#include <windows.h>
#include <shellscalingapi.h>
#include <Windowsx.h>
#include <map>

class Grid {
public:
    VOID Init(INT cx, INT cy, INT blocksize);
    VOID SetScale(float newScale);

    VOID GetSize(UINT &cx, UINT &cy);
    BOOL SizeToRect(RECT rc);
    VOID Draw(HDC hdc);

    // Adjust the grid for scroll/ ctrl+scroll
    VOID AdjustBlockSize(INT delta);
    VOID AdjustGridSize(INT delta);

private:
    float scale; // Recommended scale, based on the current DPI

    VOID RecalcAdjustedBlockSize();

    int blockSizeModifier = 7;
    int baseBlockSize;
    int adjustedBlockSize;

    int grid_cx, grid_cy;

    HBRUSH hbrGrid1, hbrGrid2;
    COLORREF rgbGrid1 = RGB(0, 51, 204); // blue
    COLORREF rgbGrid2 = RGB(204, 153, 0); // dark yellow
};

extern  PROCESS_DPI_AWARENESS gpda;
__inline BOOL IsProcessPerMonitorDpiAware()  { return gpda == PROCESS_PER_MONITOR_DPI_AWARE; }
__inline BOOL IsProcessSystemDpiAware()      { return gpda == PROCESS_SYSTEM_DPI_AWARE; }

VOID InitProcessDpiAwareness();
UINT GetDpiForWindow(HWND hwnd);
BOOL EnableNonClientScalingForWindow(HWND hwnd);
BOOL GetWindowSizeForClientSize(HWND hwnd, UINT &cx, UINT &cy);

class DPIContext
{
public:
    DPIContext()           { DPI = 96; }
    VOID SetDPI(UINT _DPI) { DPI = _DPI; }
    UINT GetDPI()          { return DPI; }

    INT ScaleLogToPhys(INT val) { return MulDiv(val, DPI, 96); }
    INT ScalePhysToLog(INT val) { return MulDiv(val, 96, DPI); }

    UINT GetPercentage() { return ScaleLogToPhys(100); }
    float GetFloat()     { return ((float)DPI / 96); }

private:
    UINT DPI;
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
    DPIContext DPI;

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

    // Default Settings
    const Settings DefSettings = {
        TRUE,  //Resize;
        FALSE, //Snap Window To Grid;
        TRUE,  //Restrict To Monitor Size;
        FALSE  //Always Entirely On Monitor
    };

    // Each window has it's own Grid
    Grid grid;

    // Default Grid Size
    const INT DefGridCX = 15,
        DefGridCY = 15,
        DefGridBlockSize = 50;

    // Each window tracks if it's in the process of handling
    // WM_DPICHANGED, in order to error if a DPI change causes 
    // the grid to be resized.
    BOOL bHandlingDpiChange = FALSE;

    // In order to behave correctly while the window is
    // snapped to corners/ sides of monitors, keep track
    // of whether the window is 'snapped'.
    // TODO: How are windows supposed to know if they are 'snapped'?
    BOOL bTrackSnap = FALSE;
    BOOL IsSnapped();

    // Delta accumulater for WM_MOUSEWHEEL
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
    MWdelta mw_delta;

    // Syntax Glue to allow a single WndProc to be used for several windows at a time
    static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
    static Window* LookupWindow(HWND hwnd);
};

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

#define DbgPrintError(...) { \
        WORD PrevConsoleTextAttribs; \
        SetConsoleColor(BACKGROUND_RED | BACKGROUND_INTENSITY, PrevConsoleTextAttribs); \
        printf (__VA_ARGS__); \
        ResetConsoleColor(PrevConsoleTextAttribs); \
    }

#define DbgPrint(...) { \
        printf (__VA_ARGS__); \
    }

#define INDENT "  ---> "

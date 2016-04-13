
#include "stdafx.h"
#include "GridSample.h"

BOOL GetWindowSizeForCurrentGridSize(HWND hwnd, UINT &cx, UINT &cy)
{
    // Determine ideal client size
    UINT gridCX, gridCY;
    GetGridSize(gridCX, gridCY);

    // Use AdjustWindowRectExForDpi to get new window size
    RECT rc = { 0, 0, gridCX, gridCY };
    if (!AdjustWindowRectExForDpi_l(&rc,
        (DWORD)GetWindowLong(hwnd, GWL_STYLE),
        (DWORD)GetWindowLong(hwnd, GWL_EXSTYLE),
        FALSE, GetDpiForWindow_l(hwnd))) {
        DbgPrintError("AdjustWindowRectExForDpi Failed.\n");
        return FALSE;
    }

    // Get new window size from AdjustWindowRectExForDpi output
    cx = RECTWIDTH(rc);
    cy = RECTHEIGHT(rc);
    return TRUE;
}

VOID ResizeSuggestionRectForDpiChange(HWND hwnd, PRECT prcSuggestion)
{
    // Note: called from WM_DPICHANGED handler, and after grid has resized

    UINT windowCX, windowCY;
    if (!GetWindowSizeForCurrentGridSize(hwnd, windowCX, windowCY)) {
        return;
    }

    // Start with the current window rect
    RECT rcWindow;
    GetWindowRect(hwnd, &rcWindow);

    // Resizing the window rect should not change the monitor the window is on
    HMONITOR hmonStart = MonitorFromRect(&rcWindow, MONITOR_DEFAULTTONEAREST);
    RECT rcOrig = *prcSuggestion;

    // Adjust the suggestion rect to have the ideal width/ height
    POINT pt;
    if (GetCursorPos(&pt) && bTrackMoveSize/*PtInRect(&rcWindow, pt)*/) {
        ResizeRectAroundPoint(prcSuggestion, windowCX, windowCY, pt);
        DbgPrintHiPri("Modified suggestion rect by transforming around point!\n");

        if (!PtInRect(prcSuggestion, pt)) {
            DbgPrintError("Error, after calling ResizeRectAroundPoint, pt no longer in rect!\n");
        }

    }
    else {
        prcSuggestion->right = prcSuggestion->left + windowCX;
        prcSuggestion->bottom = prcSuggestion->top + windowCY;

        DbgPrintHiPri("Modified suggestion rect by bumping bottom right corner...\n");
    }

    // Are we about to wobble?
    HMONITOR hmonFinish = MonitorFromRect(prcSuggestion, MONITOR_DEFAULTTONEAREST);

    if (hmonStart != hmonFinish) {
        *prcSuggestion = rcOrig; // bail!
        DbgPrintError("WOBBLE ALLERT!!!\n");
    }

}

VOID HandleDpiChange(HWND hwnd, UINT DPI, RECT* prc)
{
    DbgPrintHiPri("Handling a DPI change (DPI: %i)\n", DPI);

    // Update grid with new DPI
    SetGridDpi(DPI);

    // Now that grid has updated it's size, attempt to keep the
    // new window rect from changing the grid size
    RECT rcResize = *prc;
    ResizeSuggestionRectForDpiChange(hwnd, &rcResize);

    // Adjust window size for DPI change
    SetWindowPos(hwnd, NULL,
        rcResize.left,
        rcResize.top,
        RECTWIDTH(rcResize),
        RECTHEIGHT(rcResize),
        SWP_NOZORDER | SWP_NOACTIVATE);
}

BOOL EnforceWindowPosRestrictions(PRECT prcWindow)
{
    BOOL ret = FALSE;

    // Get monitor's work area
    MONITORINFOEX mi;
    mi.cbSize = sizeof(MONITORINFOEX);
    if (!GetMonitorInfo(MonitorFromRect(prcWindow, MONITOR_DEFAULTTONEAREST), &mi)) {
        DbgPrintError("GetMonitorInfo Failed.\n");
        return FALSE;
    }
    RECT rcWork = mi.rcWork;

    // Super Hack (invisible borders) TODO: what's the magic number?
    int nudge_factor = 9;
    rcWork.left -= nudge_factor;
    rcWork.right += nudge_factor;
    rcWork.bottom += nudge_factor;

    // Restrict window size to work area size
    if (bLimitWindowSizeToMonitorSize) {
        if (PRECTWIDTH(prcWindow) > RECTWIDTH(rcWork)) {
            prcWindow->right = prcWindow->left + RECTWIDTH(rcWork);
            ret = TRUE;
        }
        if (PRECTHEIGHT(prcWindow) > RECTHEIGHT(rcWork)) {
            prcWindow->bottom = prcWindow->top + RECTHEIGHT(rcWork);
            ret = TRUE;
        }
    }

    // Ensure window is entirely in work area (but keep the current size)
    if (bEnforceEntirelyOnMonitor) {
        int cx = PRECTWIDTH(prcWindow);
        int cy = PRECTHEIGHT(prcWindow);

        if (prcWindow->left < rcWork.left) {
            prcWindow->left = rcWork.left;
            prcWindow->right = prcWindow->left + cx;
            ret = TRUE;
        }
        if (prcWindow->top < rcWork.top) {
            prcWindow->top = rcWork.top;
            prcWindow->bottom = prcWindow->top + cy;
            ret = TRUE;
        }
        if (prcWindow->right > rcWork.right) {
            prcWindow->right = rcWork.right;
            prcWindow->left = prcWindow->right - cx;
            ret = TRUE;
        }
        if (prcWindow->bottom > rcWork.bottom) {
            prcWindow->bottom = rcWork.bottom;
            prcWindow->top = prcWindow->bottom - cy;
            ret = TRUE;
        }
    }

    return ret;
}

VOID SizeWindowToGrid(HWND hwnd, PPOINT pptResizeAround)
{
    // Calc window size that would perfectly fit the grid
    UINT windowCX, windowCY;
    if (!GetWindowSizeForCurrentGridSize(hwnd, windowCX, windowCY)) {
        return;
    }

    // Start with the current window position
    RECT rcWindow;
    GetWindowRect(hwnd, &rcWindow);
    UINT prevWindowCX = RECTWIDTH(rcWindow);
    UINT prevWindowCY = RECTHEIGHT(rcWindow);

    if (pptResizeAround) {

        // Resize window so that pptResizeAround is still in the same relative position
        ResizeRectAroundPoint(&rcWindow, windowCX, windowCY, *pptResizeAround);
    } else {

        // Keep same window origin, nad nudge the bottom/right corner to the correct size
        rcWindow.right = rcWindow.left + windowCX;
        rcWindow.bottom = rcWindow.top + windowCY;
    }

    // Before moving the window, enforce window position restrictions
    if (EnforceWindowPosRestrictions(&rcWindow)) {
        DbgPrint("Nudged window position to enforce window position restrictions.\n");
    }

    // Print the change in window size
    if (prevWindowCX != windowCX || prevWindowCY != windowCY) {
        DbgPrint("Changing window size to fit grid.\n");
        DbgPrint("%sprev size : %i x %i\n%snew size: %i x %i\n",
            INDENT, windowCX, windowCY, INDENT, prevWindowCX, prevWindowCY);
    } 

    // Set new window position
    SetWindowPos(hwnd, NULL,
        rcWindow.left,
        rcWindow.top,
        RECTWIDTH(rcWindow),
        RECTHEIGHT(rcWindow),
        SWP_SHOWWINDOW);
}

VOID ApplyWindowRestrictionsForPosChanging(PWINDOWPOS pwp)
{
    RECT rcNewWindowRect = { pwp->x, pwp->y, pwp->x + pwp->cx, pwp->y + pwp->cy };
    if (EnforceWindowPosRestrictions(&rcNewWindowRect)) {
        pwp->x = rcNewWindowRect.left;
        pwp->y = rcNewWindowRect.top;
        pwp->cx = RECTWIDTH(rcNewWindowRect);
        pwp->cy = RECTHEIGHT(rcNewWindowRect);
    }

    // TODO: do something about how the cursor drifts when
    // keeping the window on the monitor while being dragged??
    // (when keeping window on monitor)

}

INT AccumulateMouseWheelDelta(WPARAM wParam) {
    static INT _delta = 0;
    INT delta = GET_WHEEL_DELTA_WPARAM(wParam);
    _delta += delta;
    if (_delta <= -120 || _delta >= 120) {
        BOOL bUp = _delta > 0;
        _delta -= (bUp ? 120 : -120);
        return (bUp ? 1 : -1);
    }
    return 0;
}

VOID HandleMouseWheel(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    INT delta = AccumulateMouseWheelDelta(wParam);
    if (delta != 0) {
        if (GET_KEYSTATE_WPARAM(wParam) & MK_CONTROL) {
            AdjustGridSize(delta);
        } else {
            AdjustBaseBlockSize(delta);
        }

        POINT ptCursor = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        SizeWindowToGrid(hwnd, &ptCursor);
    }
}

VOID Draw(HWND hwnd, HDC hdc)
{
    // Fill background color
    static COLORREF rgbBackgroundColor = RGB(0, 0, 255);
    static RECT rcClient;
    GetClientRect(hwnd, &rcClient);
    static HBRUSH hbr = CreateSolidBrush(rgbBackgroundColor);
    FillRect(hdc, &rcClient, hbr);

    // Draw Grid
    DrawGrid(hwnd, hdc);

}

VOID InitWindow(HWND hwnd)
{
    UINT DPI = GetDpiForWindow_l(hwnd);
    DbgPrintHiPri("Initializing window, DPI: %i (%i%%, %.2fx)\n",
        DPI, DPIinPercentage(DPI), DPItoFloat(DPI));

    InitGrid(hwnd);

    SizeWindowToGrid(hwnd, NULL);
}



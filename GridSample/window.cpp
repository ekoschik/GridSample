
#include "stdafx.h"
#include "GridSample.h"

BOOL Window::GetWindowSizeForCurrentGridSize(UINT &_cx, UINT &_cy)
{
    UINT cx, cy;
    grid.GetSize(cx, cy);
    if (GetWindowSizeForClientSize(hwnd, cx, cy)) {
        _cx = cx;
        _cy = cy;
        return TRUE;
    }
    return FALSE;
}

VOID Window::ResizeSuggestionRectForDpiChange(PRECT prcSuggestion)
{
    // When handling a DPI change in the middle of being snapped, we
    // should just let the snap do it's thing (so that the window is
    // always in the position that the shell/user decided to give us).
    if (bTrackSnap) {
        return;
    }

    // Determine the ideal window size we want
    UINT windowCX, windowCY;
    if (!GetWindowSizeForCurrentGridSize(windowCX, windowCY)) {
        return;
    }

    // Start with the current window rect
    RECT rcWindow;
    GetWindowRect(hwnd, &rcWindow);

    // Forcefully ensure that resizing the window rect for a DPI
    // does not change the monitor the window is on
    HMONITOR hmonStart = MonitorFromRect(&rcWindow, MONITOR_DEFAULTTONEAREST);
    RECT rcOrig = *prcSuggestion;

    // Get the current cursor position (which is NOT the position
    // at the time this message was queued, unfortunatly...)
    POINT pt;
    GetCursorPos(&pt);

    if (PtInRect(&rcWindow, pt)) {

        ResizeRectAroundPoint(prcSuggestion, windowCX, windowCY, pt);

        // TODO: do something about how cursor is still drifting
        // (maybe re-check for the cursor pos and shift if necessary?)
        
    }
    else {
        prcSuggestion->right = prcSuggestion->left + windowCX;
        prcSuggestion->bottom = prcSuggestion->top + windowCY;
    }

    // Are we about to wobble?
    HMONITOR hmonFinish = MonitorFromRect(prcSuggestion, MONITOR_DEFAULTTONEAREST);

    if (hmonStart != hmonFinish) {
        *prcSuggestion = rcOrig; // bail!
        DbgPrintError("WOBBLE ALLERT!!!\n");

        // TODO: do something better here... can we attempt to move
        // the ideal rect towords the original suggestion until they
        // are on the same monitor???

    }

}

BOOL Window::EnforceWindowPosRestrictions(PRECT prcWindow)
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
    if (settings.bRestrictToMonitorSize) {

        // TODO: recognize which side should be modified (aka, if resizing, which side is being resized?)

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
    if (settings.bAlwaysEntirelyOnMonitor) {
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

VOID Window::SizeWindowToGrid(PPOINT pptResizeAround)
{
    // Calc window size that would perfectly fit the grid
    UINT windowCX, windowCY;
    if (!GetWindowSizeForCurrentGridSize(windowCX, windowCY)) {
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

VOID Window::HandleDpiChange(UINT _DPI, RECT* prc)
{
    bHandlingDpiChange = TRUE;

    DbgPrint("Handling a DPI change (new DPI: %i, old DPI: %i)\n", _DPI, DPI);
    DPI = DPI;

    // Update grid with new DPI
    grid.SetDpi(DPI);

    // Just take the suggestion rect
    RECT rcResize = *prc;
    //ResizeSuggestionRectForDpiChange(&rcResize);

    // Adjust window size for DPI change
    SetWindowPos(hwnd, NULL,
        rcResize.left,
        rcResize.top,
        RECTWIDTH(rcResize),
        RECTHEIGHT(rcResize),
        SWP_NOZORDER | SWP_NOACTIVATE);

    bHandlingDpiChange = FALSE;
}

VOID Window::HandleWindowPosChanged()
{
    if (grid.SizeToWindow(hwnd) && bHandlingDpiChange) {
        DbgPrintError("Error: resized grid while handling a DPI change.\n");
    }
}

VOID Window::HandleWindowPosChanging(PWINDOWPOS pwp)
{
    // Hack Alert! using a private win32k window arrangement bit...
    bTrackSnap = pwp->flags & 0x00100000; // TODO: get rid of this nonsense

    RECT rcNewWindowRect = { pwp->x, pwp->y, pwp->x + pwp->cx, pwp->y + pwp->cy };
    if (EnforceWindowPosRestrictions(&rcNewWindowRect)) {
        pwp->x = rcNewWindowRect.left;
        pwp->y = rcNewWindowRect.top;
        pwp->cx = RECTWIDTH(rcNewWindowRect);
        pwp->cy = RECTHEIGHT(rcNewWindowRect);
    }
}

VOID Window::HandleEnterExitMoveSize(BOOL bEnter)
{
    if (bEnter) {
        bTrackMoveSize = TRUE;
    } else {
        if (settings.bSnapWindowToGrid && !bTrackSnap) {
            SizeWindowToGrid(NULL);
        }
        bTrackMoveSize = FALSE;
    }
}

VOID Window::HandleMouseWheel(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    INT wheel_delta;
    if ((wheel_delta = mw_delta.inc(GET_WHEEL_DELTA_WPARAM(wParam))) != 0) {

        if (GET_KEYSTATE_WPARAM(wParam) & MK_CONTROL) {
            grid.AdjustGridSize(wheel_delta);
        } else {
            grid.AdjustBlockSize(wheel_delta);
        }

        POINT ptCursor = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        SizeWindowToGrid(&ptCursor);
    }
}

VOID Window::HandleDoubleClick(POINT pt)
{
    SizeWindowToGrid(&pt);
}

VOID Window::Draw(HDC hdc)
{
    // Fill background color
    static COLORREF rgbBackgroundColor = RGB(0, 0, 255);
    static RECT rcClient;
    GetClientRect(hwnd, &rcClient);
    static HBRUSH hbr = CreateSolidBrush(rgbBackgroundColor);
    FillRect(hdc, &rcClient, hbr);

    // Draw Grid
    grid.Draw(hdc);
}

VOID Window::Create(HWND _hwnd)
{
    hwnd = _hwnd;

    DPI = GetDpiForWindow(hwnd);
    DbgPrint("Initializing window, DPI: %i (%i%%, %.2fx)\n",
        DPI, DPIinPercentage(DPI), DPItoFloat(DPI));

    grid.SetDpi(DPI);
    SizeWindowToGrid(NULL);

    EnableNonClientScalingForWindow(hwnd);
}



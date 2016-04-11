
#include "stdafx.h"
#include "GridSample.h"

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
    int cx = PRECTWIDTH(prcWindow);
    int cy = PRECTHEIGHT(prcWindow);
    if (bEnforceEntirelyOnMonitor) {
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

BOOL GetWindowSizeForDesiredClientSize(
    HWND hwnd,
    UINT clientX,
    UINT clientY,
    UINT &windowX,
    UINT &windowY)
{
    RECT rc = { 0, 0, clientX, clientY };

    if (AdjustWindowRectExForDpi_l(&rc,
        (DWORD)GetWindowLong(hwnd, GWL_STYLE),
        (DWORD)GetWindowLong(hwnd, GWL_EXSTYLE),
        FALSE, GetDpiForWindow_l(hwnd))) {

        windowX = RECTWIDTH(rc);
        windowY = RECTHEIGHT(rc);
        return TRUE;
    }

    return FALSE;
}

VOID AdjustWindowSizeAroundPoint(PRECT prcWindow, POINT pt, UINT windowCX, UINT windowCY)
{
    prcWindow->left = pt.x - MulDiv(pt.x - prcWindow->left, windowCX, PRECTWIDTH(prcWindow));
    prcWindow->top = pt.y - MulDiv(pt.y - prcWindow->top, windowCY, PRECTHEIGHT(prcWindow));
    prcWindow->right = prcWindow->left + windowCX;
    prcWindow->bottom = prcWindow->top + windowCY;
}

VOID SizeWindowToGrid(HWND hwnd, PPOINT pptResizeAround)
{
    // Determine ideal client size
    UINT gridCX, gridCY;
    GetGridSize(gridCX, gridCY);

    // Use AdjustWindowRectExForDpi to get new window size
    UINT windowCX, windowCY;
    if (!GetWindowSizeForDesiredClientSize(hwnd, gridCX, gridCY, windowCX, windowCY)) {
        DbgPrintError("Failed to determine ideal window size.\n");
        return;
    }

    // Print the change in window size
    RECT rcWindow;
    GetWindowRect(hwnd, &rcWindow);
    UINT prevWindowCX = RECTWIDTH(rcWindow);
    UINT prevWindowCY = RECTHEIGHT(rcWindow);

    // Adjust window size, either around the cursor, or by nudging the bottom/right corner
    if (pptResizeAround) {
        AdjustWindowSizeAroundPoint(&rcWindow, *pptResizeAround, windowCX, windowCY);
    } else {
        rcWindow.right = rcWindow.left + windowCX;
        rcWindow.bottom = rcWindow.top + windowCY;
    }

    // Before moving the window, enforce window position restrictions
    if (EnforceWindowPosRestrictions(&rcWindow)) {
        DbgPrint("Nudged window position to keep window on current monitor.\n");
    }

    if (prevWindowCX != windowCX || prevWindowCY != windowCY) {
        DbgPrint("Changing window size to fit grid.\n   -->prev size: %i x %i\n   -->new size: %i x %i\n",
            windowCX, windowCY, prevWindowCX, prevWindowCY);
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

        DbgPrint("Restricted resize while handling WINDOWPOSCHANGING.\n");
    }
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

VOID HandleDpiChange(HWND hwnd, UINT DPI, RECT* prc)
{
    DbgPrint("Handling a DPI change (DPI: %i)\n", DPI);

    // Update grid with new DPI
    SetGridDpi(DPI);

    // Adjust window size for DPI change
    SetWindowPos(hwnd, NULL,
        prc->left, prc->top,
        prc->right - prc->left,
        prc->bottom - prc->top,
        SWP_NOZORDER | SWP_NOACTIVATE);
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
    DbgPrint("Initializing window, DPI: %i (%i%%, %.2fx)\n",
        DPI, DPIinPercentage(DPI), DPItoFloat(DPI));

    InitGrid(hwnd);

    SizeWindowToGrid(hwnd, NULL);
}



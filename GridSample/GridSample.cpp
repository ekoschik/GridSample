
#include "stdafx.h"
#include <windows.h>
#include "GridSample.h"
#include <Windowsx.h>


BOOL bEnforceNoLargerThanWorkArea = FALSE;
BOOL bEnforceEntirelyOnCurrentWorkArea = FALSE;
BOOL bEnforceWindowPosWhilePosChanging = TRUE;


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
    if (bEnforceNoLargerThanWorkArea) {
        if (PRECTWIDTH(prcWindow) > RECTWIDTH(rcWork)) {
            prcWindow->right = prcWindow->left + RECTWIDTH(rcWork);
            ret = TRUE;
        }
        if (PRECTHEIGHT(prcWindow) > RECTHEIGHT(rcWork)) {
            prcWindow->bottom = prcWindow->top + RECTHEIGHT(rcWork);
            ret = TRUE;
        }
    }

    // Keep the window size from this point
    int cx = PRECTWIDTH(prcWindow);
    int cy = PRECTHEIGHT(prcWindow);

    // Ensure window is entirely in work area
    if (bEnforceEntirelyOnCurrentWorkArea) {
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

VOID AdjustWindowSizeAroundPoint(PRECT prcWindow, POINT pt)
{

    // Goal: adjust rcWindow to have new ideal window size,
    // while keeping the window on the current monitor,
    // and under the cursor

    // Get the current cursor offset from the window's origin
    //int dx = pptResizeAround->x - rcWindow.left;
    //int dy = pptResizeAround->y - rcWindow.top;


    //dx *= windowCX / (float)RECTWIDTH(rcWindow);
    //dy *= windowCY / (float)RECTHEIGHT(rcWindow);
    //
    //rcWindow.left = pptResizeAround->x - dx;
    //rcWindow.top = pptResizeAround->y - dy;
    //rcWindow.right = rcWindow.left + windowCX;
    //rcWindow.bottom = rcWindow.top + windowCY;
    //
    //// Error if the cursor isn't within the new window size
    //if (!PtInRect(&rcWindow, *pptResizeAround)) {
    //    DbgPrintError("Failed to keep cursor in window while resizing.\n");
    //}
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
        AdjustWindowSizeAroundPoint(&rcWindow, *pptResizeAround);
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

VOID HandleMouseWheel(HWND hwnd, INT delta, BOOL bControl, POINT ptCursorScreen)
{
    if (bControl) {
        AdjustGridSize(delta);
    } else {
        AdjustBaseBlockSize(delta);
    }

    SizeWindowToGrid(hwnd, &ptCursorScreen);
}

VOID InitWindow(HWND hwnd)
{
    UINT DPI = GetDpiForWindow_l(hwnd);
    DbgPrint("Initializing window, DPI: %i (%i%%, %.2fx)\n",
        DPI, DPIinPercentage(DPI), DPItoFloat(DPI));
    
    InitGrid(hwnd);

    SizeWindowToGrid(hwnd, NULL);
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

BOOL bTrackSnap = FALSE;
BOOL bHandlingDpiChange = FALSE;
LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {

    case WM_CREATE:
        InitWindow(hwnd);
        break;

    case WM_DPICHANGED:
        bHandlingDpiChange = TRUE;
        HandleDpiChange(hwnd, HIWORD(wParam), (RECT*)lParam);
        bHandlingDpiChange = FALSE;
        break;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        Draw(hwnd, hdc);
        EndPaint(hwnd, &ps);
        break;
    }

    case WM_WINDOWPOSCHANGING:
    {
        PWINDOWPOS pwp = (WINDOWPOS*)lParam;

        // TODO look up this hack bit that's really being checked here
        bTrackSnap = pwp->flags > 1000;

        if (bEnforceWindowPosWhilePosChanging) {
            RECT rcNewWindowRect = { pwp->x, pwp->y, pwp->x + pwp->cx, pwp->y + pwp->cy };
            if (EnforceWindowPosRestrictions(&rcNewWindowRect)) {
                pwp->x = rcNewWindowRect.left;
                pwp->y = rcNewWindowRect.top;
                pwp->cx = RECTWIDTH(rcNewWindowRect);
                pwp->cy = RECTHEIGHT(rcNewWindowRect);

                DbgPrint("Restricted resize while handling WINDOWPOSCHANGING.\n");
            }
        }
        break;
    }
    
    case WM_WINDOWPOSCHANGED:
    {
        // Resize the grid to the new window size
        RECT rcClient;
        GetClientRect(hwnd, &rcClient);
        if (SizeGridToWindow(rcClient) && bHandlingDpiChange) {
            DbgPrintError("Error: resized grid while handling a DPI change.");
        }
        break;
    }

    case WM_EXITSIZEMOVE:

        // If being resized, but not snapped, resize the window
        if (!bTrackSnap) {
            SizeWindowToGrid(hwnd, NULL);
        }
        break;

    case WM_MOUSEWHEEL:
    {
        BOOL bControl = GET_KEYSTATE_WPARAM(wParam) & MK_CONTROL;
        
        INT delta = GET_WHEEL_DELTA_WPARAM(wParam);
        static INT _delta = 0;
        _delta += delta;

        if (_delta <= -120 || _delta >= 120) {
            BOOL bUp = _delta > 0;
            _delta -= (bUp ? 120 : -120);

            POINT ptCursor = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };

            HandleMouseWheel(hwnd, (bUp ? 1 : -1), bControl, ptCursor);
        }
    }
    break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    }

    return DefWindowProc(hwnd, message, wParam, lParam);
}

HWND CreateMainWindow()
{
    LPWSTR WndClassName = _T("WndClass");
    LPWSTR WndTitle = _T("Grid Sample");

    UINT WndClassStyle = CS_HREDRAW | CS_VREDRAW /*| CS_DBLCLKS*/;
    DWORD WndStyleEx = 0;
    DWORD WndStyle = WS_OVERLAPPEDWINDOW 
        
        // removing the following dis-allows resizing the window
        /*^ (WS_THICKFRAME | WS_MAXIMIZEBOX | WS_MINIMIZEBOX)*/;

    HINSTANCE hinst = GetModuleHandle(NULL);
    WNDCLASSEX wcex = { 0 };
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = WndClassStyle;
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = hinst;
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.lpszClassName = WndClassName;
    if (!RegisterClassEx(&wcex)) {
        DbgPrintError("Error: RegisterClassEx Failed.\n");
        return NULL;
    }

    int x = CW_USEDEFAULT,
        y = CW_USEDEFAULT,
        cx = CW_USEDEFAULT,
        cy = CW_USEDEFAULT;

    HWND hwnd = CreateWindowEx(
        WndStyleEx,
        WndClassName,
        WndTitle,
        WndStyle,
        x, y, cx, cy,
        nullptr, nullptr, hinst, nullptr);

    if (!hwnd) {
        DbgPrintError("Error: CreateWindowEx Failed.\n");
        return NULL;
    }

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    return hwnd;
}

int main()
{
    BOOL bPMDpiAware = InitProcessDpiAwareness();

    HWND hwnd = CreateMainWindow();

    EnableNonClientScalingForWindow(hwnd);

    LPCWSTR WindowTitle = bPMDpiAware ? _T("Grid Sample (PM)") : _T("Grid Sample (Sys)");
    SetWindowText(hwnd, WindowTitle);

    DbgPrint("Entering Message Loop.\n");

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return msg.wParam;
}


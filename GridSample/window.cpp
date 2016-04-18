
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
    int nudge_factor = 12;
    rcWork.top -= nudge_factor;
    rcWork.left -= nudge_factor;
    rcWork.right += nudge_factor;
    rcWork.bottom += nudge_factor;

    // Restrict window size to work area size
    if (settings.bRestrictToMonitorSize) {
        if (PRECTWIDTH(prcWindow) > RECTWIDTH(rcWork)) {
            DbgPrint("Restricting window width to monitor work area width.\n");
            DbgPrint("%swas %i, setting to %i.\n", INDENT, PRECTWIDTH(prcWindow), RECTWIDTH(rcWork));
            prcWindow->right = prcWindow->left + RECTWIDTH(rcWork);
            ret = TRUE;
        }
        if (PRECTHEIGHT(prcWindow) > RECTHEIGHT(rcWork)) {
            DbgPrint("Restricting window height to monitor work area height.\n");
            DbgPrint("%swas %i, setting to %i.\n", INDENT, PRECTHEIGHT(prcWindow), RECTHEIGHT(rcWork));
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

    RECT rcWindow;
    GetWindowRect(hwnd, &rcWindow);
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

    // Bail (use the original rect) if the new one is on a different monitor
    if (hmonStart != MonitorFromRect(prcSuggestion, MONITOR_DEFAULTTONEAREST)) {
        *prcSuggestion = rcOrig;
        DbgPrintError("WOBBLE ALLERT!!!\n");

        // TODO: do something better here... can we attempt to move
        // the ideal rect towords the original suggestion until they
        // are on the same monitor???

    }

}

VOID Window::HandleDpiChange(UINT _DPI, RECT* prc)
{
    bHandlingDpiChange = TRUE;

    DbgPrint("Handling a DPI change (new DPI: %i, old DPI: %i)\n", _DPI, DPI);
    DPI = _DPI;

    // Update grid with new DPI
    grid.SetDpi(DPI);

    // Start with the suggestion rect
    RECT rcResize = *prc;
    
    ResizeSuggestionRectForDpiChange(&rcResize);

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
    if (!IsZoomed(hwnd) && // (if not maximized)
        EnforceWindowPosRestrictions(&rcNewWindowRect)) {

        pwp->flags |= SWP_NOMOVE;

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
    INT wheel_delta = mw_delta.inc(GET_WHEEL_DELTA_WPARAM(wParam));

    if (wheel_delta != 0) {

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


std::map<HWND, Window*> WindowMap;
LRESULT CALLBACK Window::WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    std::map<HWND, Window*>::iterator it;
#define LOOKUPWINDOW    (it = WindowMap.find(hwnd)) != WindowMap.end()
#define WINDOWPTR       (it->second)

    switch (message) {

    case WM_CREATE:
    {
        Window* pWindow = (Window*)(((CREATESTRUCT*)lParam)->lpCreateParams);
        if (pWindow) {
            WindowMap.insert(std::pair<HWND, Window*>(hwnd, pWindow));
            pWindow->Create(hwnd);
        }
        break;
    }

    case WM_DPICHANGED:
        if (LOOKUPWINDOW) {
            WINDOWPTR->HandleDpiChange(HIWORD(wParam), (RECT*)lParam);
        }
        break;

    case WM_PAINT:
        if (LOOKUPWINDOW) {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            WINDOWPTR->Draw(hdc);
            EndPaint(hwnd, &ps);
        }
        break;

    case WM_WINDOWPOSCHANGING:
        if (LOOKUPWINDOW) {
            WINDOWPTR->HandleWindowPosChanging((WINDOWPOS*)lParam);
        }
        break;

    case WM_WINDOWPOSCHANGED:
        if (LOOKUPWINDOW) {
            WINDOWPTR->HandleWindowPosChanged();
        }
        break;

    case WM_ENTERSIZEMOVE:
    case WM_EXITSIZEMOVE:
        if (LOOKUPWINDOW) {
            WINDOWPTR->HandleEnterExitMoveSize(message == WM_ENTERSIZEMOVE);
        }
        break;

    case WM_MOUSEWHEEL:
        if (LOOKUPWINDOW) {
            WINDOWPTR->HandleMouseWheel(hwnd, wParam, lParam);
        }
        break;

    case WM_LBUTTONDBLCLK:
        if (LOOKUPWINDOW) {
            POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            WINDOWPTR->HandleDoubleClick(pt);
        }
        break;

    case WM_DESTROY:
        WindowMap.erase(hwnd);
        if (WindowMap.size() == 0) {
            PostQuitMessage(0);
        }
        DbgPrint("Window Exiting...\n");
        break;
    }

    return DefWindowProc(hwnd, message, wParam, lParam);
}

BOOL Window::EnsureWindowIsRegistered(HINSTANCE hinst, LPWSTR WndClassName)
{
    // It is only necessary to register the window class once
    static BOOL bRegistered = FALSE;
    if (!bRegistered) {
        UINT WndClassStyle = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
        WNDCLASSEX wcex = { 0 };
        wcex.cbSize = sizeof(WNDCLASSEX);
        wcex.style = WndClassStyle;
        wcex.lpfnWndProc = WndProc;
        wcex.hInstance = hinst;
        wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wcex.lpszClassName = WndClassName;
        if (RegisterClassEx(&wcex)) {
            bRegistered = TRUE;
        }
    }
    return bRegistered;
}

Window::Window(INT Gridcx, INT Gridcy, INT blocksize, Settings _settings)
{
    settings = _settings;
    grid.Init(Gridcx, Gridcy, blocksize);

    LPWSTR WndClassName = _T("WndClass");
    LPWSTR WndTitle = _T("Grid Sample");
    HINSTANCE hinst = GetModuleHandle(NULL);

    if (!EnsureWindowIsRegistered(hinst, WndClassName)) {
        DbgPrintError("Error: RegisterClassEx Failed.\n");
        return;
    }

    DWORD WndStyleEx = 0;
    DWORD WndStyle = settings.bResize ? WS_OVERLAPPEDWINDOW :
        WS_OVERLAPPEDWINDOW ^ (WS_THICKFRAME | WS_MAXIMIZEBOX | WS_MINIMIZEBOX);

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
        nullptr, nullptr, hinst,
        this);

    if (!hwnd) {
        DbgPrintError("Error: CreateWindowEx Failed.\n");
        return;
    }

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
}


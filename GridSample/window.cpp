
#include "stdafx.h"
#include "GridSample.h"

std::map<HWND, Window*> WindowMap;

BOOL Window::IsSnapped()
{
    // Windows sends a private flag in WINDOWPOSCHANGING, which is
    // set when the window is getting snapped to a monitor side.
    // Without knowing whether or not the window is in this state,
    // some window size restrictions will cause the window to not
    // behave correctly when being snapped, and so care is taken to
    // skip enforcing the size restrictions when in this state.
    return bTrackSnap || IsZoomed(hwnd);
}

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
    int nudge_factor = 12; // I think I need to calc this using 'unadjustwindowrectfordpi...'
    rcWork.top -= nudge_factor;
    rcWork.left -= nudge_factor;
    rcWork.right += nudge_factor;
    rcWork.bottom += nudge_factor;

    // Restrict window size to work area size
    if (settings.bRestrictToMonitorSize) {
        if (PRECTWIDTH(prcWindow) > RECTWIDTH(rcWork)) {
            prcWindow->right = prcWindow->left + RECTWIDTH(rcWork);
            ret = TRUE;
        }
        if (PRECTHEIGHT(prcWindow) > RECTHEIGHT(rcWork)) {
            prcWindow->bottom = prcWindow->top + RECTHEIGHT(rcWork);
            ret = TRUE;
        }
    }

    // Ensure window is entirely in work area
    if (settings.bAlwaysEntirelyOnMonitor) {
        KeepRectInsideBiggerRect(prcWindow, &rcWork);
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

        // Keep same window origin, and nudge the bottom/right corner to the correct size
        rcWindow.right = rcWindow.left + windowCX;
        rcWindow.bottom = rcWindow.top + windowCY;
    }

    // Before moving the window, enforce window position restrictions
    EnforceWindowPosRestrictions(&rcWindow);

    // Print the change in window size (if there is one)
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
    UINT windowCX, windowCY;
    if (!GetWindowSizeForCurrentGridSize(windowCX, windowCY)) {
        return;
    }

    RECT rcWindow;
    GetWindowRect(hwnd, &rcWindow);
    HMONITOR hmonStart = MonitorFromRect(&rcWindow, MONITOR_DEFAULTTONEAREST);
    RECT rcOrig = *prcSuggestion;

    // Get the current cursor position (which is NOT the position
    // at the time this message was queued, unfortunately...)
    POINT pt;
    GetCursorPos(&pt);

    if (PtInRect(&rcWindow, pt)) {
        ResizeRectAroundPoint(prcSuggestion, windowCX, windowCY, pt);
    } else {
        prcSuggestion->right = prcSuggestion->left + windowCX;
        prcSuggestion->bottom = prcSuggestion->top + windowCY;
    }

    // Do not change the suggestion rect if the new rect is on a different monitor
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

    DbgPrint("Handling a DPI change (new DPI: %i, old DPI: %i)\n", _DPI, DPI.GetDPI());
    DPI.SetDPI(_DPI);

    // Update grid with new DPI
    grid.SetScale(DPI.GetFloat());

    // Start with the suggestion rect
    RECT rcResize = *prc;
    
    // If we're not snapped, attempt to keep the grid from changing
    if(!IsSnapped()) {
        ResizeSuggestionRectForDpiChange(&rcResize);
    }

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
    RECT rcClient;
    GetClientRect(hwnd, &rcClient);

    // Update the grid for the new window size
    if (grid.SizeToRect(rcClient) && bHandlingDpiChange) {
        DbgPrintError("Error: resized grid while handling a DPI change.\n");
    }
}

VOID Window::HandleWindowPosChanging(PWINDOWPOS pwp)
{
    // Hack Alert! using a private win32k window arrangement bit...
    bTrackSnap = pwp->flags & 0x00100000; // TODO: get rid of this nonsense

    // If the window is not snapped, and if the new window position needs tweaking
    // (givin the current window size restrictions), overwrite the new window size,
    // and disallow the window from being further resized
    RECT rcNewWindowRect = { pwp->x, pwp->y, pwp->x + pwp->cx, pwp->y + pwp->cy };
    if (!IsSnapped() && EnforceWindowPosRestrictions(&rcNewWindowRect)) {
        pwp->flags |= SWP_NOMOVE;
        pwp->cx = RECTWIDTH(rcNewWindowRect);
        pwp->cy = RECTHEIGHT(rcNewWindowRect);
    }
}

VOID Window::HandleEnterExitMoveSize(BOOL bEnter)
{
    // When exiting the move size loop, snap the window to the grid
    // (if that setting is on), but only if the window is 'snapped'
    if (!bEnter && settings.bSnapWindowToGrid && !IsSnapped()) {
        SizeWindowToGrid(NULL);
    }
}

VOID Window::HandleMouseWheel(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    // For each tick, either adjust the overall grid size (if control is down),
    // or the block size (for normal scrolling)
    INT wheel_delta = mw_delta.inc(GET_WHEEL_DELTA_WPARAM(wParam));
    if (wheel_delta != 0) {
        if (GET_KEYSTATE_WPARAM(wParam) & MK_CONTROL) {
            grid.AdjustGridSize(wheel_delta);
        } else {
            grid.AdjustBlockSize(wheel_delta);
        }

        // After adjusting the grid size, resize the window to fit the grid,
        // but do so wrt to the cursor positon to keep the cursor within the window
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
    // Save handle to this window
    hwnd = _hwnd;

    // Turn on (private) title bar DPI scaling for the window
    EnableNonClientScalingForWindow(hwnd);

    // Determine the initial window DPI
    DPI.SetDPI(GetDpiForWindow(hwnd));
    DbgPrint("%sStarting window DPI: %i (%i%%, %.2fx)\n",
        INDENT, DPI.GetDPI(), DPI.GetPercentage(), DPI.GetFloat());

    // Initialize grid at the starting DPI
    grid.SetScale(DPI.GetFloat());

    // Size window so that it always starts perfectly fitting the grid
    SizeWindowToGrid(NULL);
}

Window* Window::LookupWindow(HWND hwnd)
{
    std::map<HWND, Window*>::iterator it = WindowMap.find(hwnd);
    return (it != WindowMap.end() ? it->second : NULL);
}

LRESULT CALLBACK Window::WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    Window* pWindow;

    switch (message) {

    case WM_CREATE:

        // Now that the window has an HWND, add the window to the map, and call Create
        if (pWindow = (Window*)(((CREATESTRUCT*)lParam)->lpCreateParams)) {
            WindowMap.insert(std::pair<HWND, Window*>(hwnd, pWindow));
            pWindow->Create(hwnd);
        }
        break;

    case WM_DESTROY:

        // When each window is destroyed, remove from the map
        WindowMap.erase(hwnd);

        // If no more windows are around, exit the message loop
        if (WindowMap.size() == 0) {
            DbgPrint("Final Window Exiting...\n");
            PostQuitMessage(0);
        }
        else {
            DbgPrint("Window Exiting...\n");
        }

        break;

    case WM_PAINT:
        if (pWindow = LookupWindow(hwnd)) {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            pWindow->Draw(hdc);
            EndPaint(hwnd, &ps);
        }
        break;

    case WM_DPICHANGED:
        if (pWindow = LookupWindow(hwnd)) {
            pWindow->HandleDpiChange(HIWORD(wParam), (RECT*)lParam);
        }
        break;

    case WM_WINDOWPOSCHANGING:
        if (pWindow = LookupWindow(hwnd)) {
            pWindow->HandleWindowPosChanging((WINDOWPOS*)lParam);
        }
        break;

    case WM_WINDOWPOSCHANGED:
        if (pWindow = LookupWindow(hwnd)) {
            pWindow->HandleWindowPosChanged();
        }
        break;

    case WM_ENTERSIZEMOVE:
    case WM_EXITSIZEMOVE:
        if (pWindow = LookupWindow(hwnd)) {
            pWindow->HandleEnterExitMoveSize(message == WM_ENTERSIZEMOVE);
        }
        break;

    case WM_MOUSEWHEEL:
        if (pWindow = LookupWindow(hwnd)) {
            pWindow->HandleMouseWheel(hwnd, wParam, lParam);
        }
        break;

    case WM_LBUTTONDBLCLK:
        if (pWindow = LookupWindow(hwnd)) {
            POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            pWindow->HandleDoubleClick(pt);
        }
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
            DbgPrint("Registered window class.\n");
            bRegistered = TRUE;
        } else {
            DbgPrintError("Failed to register window class!\n");
        }
    }
    return bRegistered;
}

VOID Window::Init(
    INT Gridcx,
    INT Gridcy,
    INT blocksize,
    BOOL bResize,
    BOOL bSnapWindowToGrid,
    BOOL bRestrictToMonitorSize,
    BOOL bAlwaysEntirelyOnMonitor)
{
    DbgPrint("Creating window...\n");

    // Save Settings and Print On/Off for each Setting
    Settings set = { bResize,
                     bSnapWindowToGrid,
                     bRestrictToMonitorSize,
                     bAlwaysEntirelyOnMonitor };
    settings = set;

#define StrOnOff(on)    (on ? "ON" : "OFF")
    DbgPrint("%sResize: %s\n", INDENT, StrOnOff(bResize));
    DbgPrint("%sSnap Window to Grid: %s\n", INDENT, StrOnOff(bSnapWindowToGrid));
    DbgPrint("%sRestrict Window to Monitor Size: %s\n", INDENT, StrOnOff(bRestrictToMonitorSize));
    DbgPrint("%sKeep Window Entirely on Monitor: %s\n", INDENT, StrOnOff(bAlwaysEntirelyOnMonitor));
    
    // Initialize Grid
    grid.Init(Gridcx, Gridcy, blocksize);
    DbgPrint("%sInitialized grid to size %i x %i (blocksize: %i)\n", INDENT, Gridcx, Gridcy, blocksize);

    // Register window class (one time only)
    LPWSTR WndClassName = _T("WndClass");
    LPWSTR WndTitle = _T("Grid Sample");
    HINSTANCE hinst = GetModuleHandle(NULL);
    if (!EnsureWindowIsRegistered(hinst, WndClassName)) {
        return;
    }

    // Create Window
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
                    
                    // Create each window with a pointer to it's Window object
                    this);

    if (!hwnd) {
        DbgPrintError("Error: CreateWindowEx Failed.\n");
        return;
    }

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
}

Window::Window(
    INT cx,
    INT cy,
    INT blocksize,
    BOOL bResize,
    BOOL bSnapWindowToGrid,
    BOOL bRestrictToMonitorSize,
    BOOL bAlwaysEntirelyOnMonitor)
{
    Init(cx, cy, blocksize,
         bResize,
         bSnapWindowToGrid,
         bRestrictToMonitorSize,
         bAlwaysEntirelyOnMonitor);
}

VOID Window::Init(INT cx, INT cy, INT blocksize)
{
    Init(cx, cy, blocksize,
        DefSettings.bResize,
        DefSettings.bSnapWindowToGrid,
        DefSettings.bRestrictToMonitorSize,
        DefSettings.bAlwaysEntirelyOnMonitor);
}

Window::Window(INT cx, INT cy, INT blocksize)
{
    Init(cx, cy, blocksize);
}

Window::Window()
{
    Init(DefGridCX, DefGridCY, DefGridBlockSize);
}

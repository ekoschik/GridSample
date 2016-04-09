
#include "stdafx.h"
#include <windows.h>
#include "GridSample.h"

WORD gPrevConsoleTextAttribs;



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


VOID SizeWindowToGrid(HWND hwnd)
{
    UINT gridCX, gridCY;
    GetGridSize(gridCX, gridCY);

    // Return early if current client size is already the correct size
    RECT rcClient;
    GetClientRect(hwnd, &rcClient);
    if (RECTWIDTH(rcClient) == gridCX && RECTHEIGHT(rcClient) == gridCY) {
        return;
    }

    // Use AdjustWindowRectExForDpi to get new window size
    UINT windowCX, windowCY;
    if (!GetWindowSizeForDesiredClientSize(hwnd, gridCX, gridCY, windowCX, windowCY)) {
        DbgPrintError("Failed to determine ideal window size.\n");
        return;
    }

    // Print the change in window size
    RECT rcWindow;
    GetWindowRect(hwnd, &rcWindow);
    DbgPrint("Snapping window size to grid size\n\told size: %i x %i\n\tnew size: %i x %i\n",
        RECTWIDTH(rcWindow), RECTHEIGHT(rcWindow), windowCX, windowCY);

    // Nudge out (or in) the bottom right corner to correct the window size
    rcWindow.right = rcWindow.left + windowCX;
    rcWindow.bottom = rcWindow.top + windowCY;

    // Set new window position
    SetWindowPos(hwnd, NULL,
        rcWindow.left,
        rcWindow.top,
        RECTWIDTH(rcWindow),
        RECTHEIGHT(rcWindow),
        SWP_SHOWWINDOW);

    // Error if the new client size doesn't match the grid size
    GetClientRect(hwnd, &rcClient);
    if (gridCX != RECTWIDTH(rcClient) ||
        gridCY != RECTHEIGHT(rcClient)) {

        DbgPrintError("Error, after setting initial window pos, client size didn't match expected.\n");
        DbgPrintError("-->wanted %i x %i\n", gridCX, gridCY);
        DbgPrintError("-->actual %i x %i\n", RECTWIDTH(rcClient), RECTHEIGHT(rcClient));
    }


    // Determine window size to fit grid
    RECT rc = { 0, 0, gridCX, gridCY };
    if (AdjustWindowRectExForDpi_l(&rc,
        (DWORD)GetWindowLong(hwnd, GWL_STYLE),
        (DWORD)GetWindowLong(hwnd, GWL_EXSTYLE),
        FALSE, GetDpiForWindow_l(hwnd))) {

        UINT cx = RECTWIDTH(rc);
        UINT cy = RECTHEIGHT(rc);
        DbgPrint("Calculated window size: %i x %i\n", cx, cy);

        // Extend current window rect to new size
        GetWindowRect(hwnd, &rc);
        rc.right = rc.left + cx;
        rc.bottom = rc.top + cy;
        DbgPrint("Setting new window position... [%i, %i, %i, %i]\n",
            rc.left, rc.top, rc.right, rc.bottom);

        // Set new window position
        SetWindowPos(hwnd, NULL,
            rc.left,
            rc.top,
            RECTWIDTH(rc),
            RECTHEIGHT(rc),
            SWP_SHOWWINDOW);

        // Error if the new client size doesn't match the grid size
        GetClientRect(hwnd, &rcClient);
        if (gridCX != RECTWIDTH(rcClient) ||
            gridCY != RECTHEIGHT(rcClient)) {

            DbgPrintError("Error, after setting initial window pos, client size didn't match expected.\n");
            DbgPrintError("-->wanted %i x %i\n", gridCX, gridCY);
            DbgPrintError("-->actual %i x %i\n", RECTWIDTH(rcClient), RECTHEIGHT(rcClient));

            // Sanity check (did we not get the correct window size?
            RECT rcWindow;
            GetWindowRect(hwnd, &rcWindow);
            if (!EqualRect(&rcWindow, &rc)) {
                DbgPrintError("maybe setting initial window position didn't take?\n");
                DbgPrintError("-->wanted %i x %i\n", RECTWIDTH(rc), RECTHEIGHT(rc));
                DbgPrintError("-->actual %i x %i\n", RECTWIDTH(rcWindow), RECTHEIGHT(rcWindow));
            }
        }
    }
}

VOID InitWindow(HWND hwnd)
{
    UINT DPI = GetDpiForWindow_l(hwnd);
    DbgPrint("Initializing window, DPI: %i (%i%%, %.2fx)\n",
        DPI, DPIinPercentage(DPI), DPItoFloat(DPI));
    
    InitGrid(hwnd);

    SizeWindowToGrid(hwnd);
}

VOID HandleDpiChange(HWND hwnd, UINT DPI, RECT* prc)
{
    static UINT gDPI = 96;
    DbgPrint("Handling a DPI change (new: %i, old: %i)\n",
        DPI, gDPI);
    gDPI = DPI;

    // Update grid with new DPI
    SetGridDpi(DPI);

    // Adjust window size for DPI change
    SetWindowPos(hwnd, NULL,
                 prc->left, prc->top,
                 prc->right - prc->left,
                 prc->bottom - prc->top,
                 SWP_NOZORDER | SWP_NOACTIVATE);
}

VOID HandleMinMaxInfo(PMINMAXINFO pmmi)
{

    // TODO: limit the size of the window to the current monitor's work area











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

    case WM_EXITSIZEMOVE:
    case WM_WINDOWPOSCHANGING:
    case WM_WINDOWPOSCHANGED:
    {
        // WindowPosChanging has some secret high bit we can use to
        // hint whether or not we're in the process of being snapped
        if (message == WM_WINDOWPOSCHANGING) {
            // TODO look up the bit that's being set
            bTrackSnap = ((WINDOWPOS*)lParam)->flags > 1000;
        }

        // Resize the grid to the new window size
        RECT rcClient;
        GetClientRect(hwnd, &rcClient);
        if (SizeGridToWindow(rcClient) && bHandlingDpiChange) {
            DbgPrintError("Error: resized grid while handling a DPI change.");
        }

        // If the last WindowPosChanging didn't looked like a snap,
        // snap the window size to fit the grid
        if (message == WM_EXITSIZEMOVE && !bTrackSnap) {
            SizeWindowToGrid(hwnd);
        }
        break;
    }

    case WM_GETMINMAXINFO:
        HandleMinMaxInfo((PMINMAXINFO)lParam);
        break;

    case WM_MOUSEWHEEL:
        //HandleMouseWheel(hwnd,
        //    GET_KEYSTATE_WPARAM(wParam) | MK_CONTROL,
        //    GET_WHEEL_DELTA_WPARAM(wParam));
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


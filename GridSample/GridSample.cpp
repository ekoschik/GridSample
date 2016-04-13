
#include "stdafx.h"
#include "GridSample.h"


BOOL bHandlingDpiChange = FALSE;
BOOL bTrackSnap = FALSE;
BOOL bTrackMoveSize = FALSE;

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {

    case WM_CREATE:
        EnableNonClientScalingForWindow(hwnd);
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
        ApplyWindowRestrictionsForPosChanging((WINDOWPOS*)lParam);

        // Hack! TODO look up this hack bit that's really being checked here
        bTrackSnap = ((WINDOWPOS*)lParam)->flags > 1000;
        break;
    
    case WM_WINDOWPOSCHANGED:
        SizeGridToWindow(hwnd);
        break;
    
    case WM_ENTERSIZEMOVE:
        bTrackMoveSize = TRUE;
        break;

    case WM_EXITSIZEMOVE:
        if (bSnapWindowSizeToGrid && !bTrackSnap) {
            SizeWindowToGrid(hwnd, NULL);
        }
        bTrackMoveSize = FALSE;
        break;

    case WM_MOUSEWHEEL:
        HandleMouseWheel(hwnd, wParam, lParam);
        break;

    case WM_LBUTTONDBLCLK:
    {
        POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        SizeWindowToGrid(hwnd, &pt);
        break;
    }

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

    UINT WndClassStyle = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    DWORD WndStyleEx = 0;
    DWORD WndStyle = bAllowResize ? WS_OVERLAPPEDWINDOW :
        WS_OVERLAPPEDWINDOW ^ (WS_THICKFRAME | WS_MAXIMIZEBOX | WS_MINIMIZEBOX);

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

int main(int argc, char* argv[])
{
    // Read args, and return if "/?" was typed
    if (InitSettingsFromArgs(argc, argv)) {
        return 1;
    }

    // Load the DPI APIs and set the process DPI awareness
    BOOL bPMDpiAware = InitProcessDpiAwareness();

    // Create the window and set it's title based on the awareness chosen
    HWND hwnd = CreateMainWindow();
    SetWindowText(hwnd, bPMDpiAware ?
        _T("Grid Sample (PM)") :
        _T("Grid Sample (Sys)"));

    MSG msg;
    DbgPrintHiPri("Entering Message Loop.\n\n");
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}


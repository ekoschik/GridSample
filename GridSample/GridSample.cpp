
#include "stdafx.h"
#include "GridSample.h"
#include <map>

using namespace std;

map<HWND, Window*> WindowMap;

LRESULT CALLBACK Window::WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    Window* pWindow;
    map<HWND, Window*>::iterator it;
#define LOOKUPWINDOW    (it = WindowMap.find(hwnd)) != WindowMap.end()
#define WINDOWPTR       (it->second)

    switch (message) {

    case WM_CREATE:
        if (pWindow = new Window()) {
            WindowMap.insert(pair<HWND, Window*>(hwnd, pWindow));
            pWindow->Create(hwnd);
        }
        break;

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
        if (LOOKUPWINDOW) {
            WINDOWPTR->HandleEnterExitMoveSize(TRUE);
        }
        break;

    case WM_EXITSIZEMOVE:
        if (LOOKUPWINDOW) {
            WINDOWPTR->HandleEnterExitMoveSize(FALSE);
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
        PostQuitMessage(0);
        break;
    }

    return DefWindowProc(hwnd, message, wParam, lParam);
}

Window::Window()
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
        return;
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
        return;
    }

    SetWindowText(hwnd, bPMDpiAware ?
        _T("Grid Sample (PM)") :
        _T("Grid Sample (Sys)"));

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
}

int main(int argc, char* argv[])
{
    // Read args, and return immediately if "/?" was typed
    if (InitSettingsFromArgs(argc, argv)) {
        return 1;
    }

    // Load the DPI APIs and set the process DPI awareness
    bPMDpiAware = InitProcessDpiAwareness();

    // Create the window
    Window wnd;

    // Message pump
    MSG msg;
    DbgPrintHiPri("Entering Message Loop.\n\n");
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}


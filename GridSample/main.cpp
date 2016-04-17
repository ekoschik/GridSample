
#include "stdafx.h"
#include "GridSample.h"
#include <map>

using namespace std;

VOID Run()
{
    DbgPrint("Creating window 1...\n");
    Settings set1 = { TRUE,  //bResize;
                      TRUE,  //bSnapWindowToGrid;
                      FALSE, //bRestrictToMonitorSize;
                      FALSE  //bAlwaysEntirelyOnMonitor
                    };
    Window wnd1(15, // cx
                15, // cy
                50, // blocksize 
                set1);

    //DbgPrint("Creating window 2...\n");
    //Settings set2 = { TRUE,  //bResize;
    //                  FALSE, //bSnapWindowToGrid;
    //                  TRUE,  //bRestrictToMonitorSize;
    //                  FALSE  //bAlwaysEntirelyOnMonitor
    //                };
    //Window wnd2(6, // cx
    //            6, // cy
    //            45, // blocksize
    //            set2);

    // Pump Messages!
    MSG msg;
    DbgPrint("Entering Message Loop.\n\n");
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

map<HWND, Window*> WindowMap;
LRESULT CALLBACK Window::WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    map<HWND, Window*>::iterator it;
#define LOOKUPWINDOW    (it = WindowMap.find(hwnd)) != WindowMap.end()
#define WINDOWPTR       (it->second)

    switch (message) {

    case WM_CREATE:
    {
        Window* pWindow = (Window*)(((CREATESTRUCT*)lParam)->lpCreateParams);
        if (pWindow) {
            WindowMap.insert(pair<HWND, Window*>(hwnd, pWindow));
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

int main(int argc, char* argv[])
{
    InitProcessDpiAwareness();

    Run();

    DbgPrint("Thread Exiting...\n");
    return 0;
}


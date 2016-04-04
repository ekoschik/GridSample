
#include "stdafx.h"
#include "GridSample.h"

PROCESS_DPI_AWARENESS gpda;

typedef BOOL(WINAPI *fnTypeAdjustWindowRectExForDpi)(LPRECT, DWORD, BOOL, DWORD, UINT);
typedef UINT(WINAPI *fnTypeGetDpiForWindow)(HWND);
typedef BOOL(WINAPI *fnTypeProvideSuggestionRectForDpiChange)(HWND, INT, INT, LPRECT);
typedef BOOL(WINAPI *fnTypeEnableNCScaling)(HWND);
typedef BOOL(WINAPI *fnTypeEnableBroadcasting)(HWND, BOOL);

fnTypeAdjustWindowRectExForDpi pfnAdjustWindowRectExForDpi = NULL;
fnTypeGetDpiForWindow pfnGetDpiForWindow = NULL;
fnTypeEnableNCScaling fnEnableNCScaling = NULL;
fnTypeEnableBroadcasting fnEnableBroadcasting = NULL;

BOOL InitProcessDpiAwareness()
{
    HMODULE hModUser32 = GetModuleHandle(_T("user32.dll"));

    // Load AdjustWindowRectExForDpi (which was originally exported by ordinal)
    pfnAdjustWindowRectExForDpi =
        (fnTypeAdjustWindowRectExForDpi)GetProcAddress(hModUser32, (LPCSTR)2580);
    if (pfnAdjustWindowRectExForDpi) {
        DbgPrint("Found AdjustWindowRectExForDpi (by ordinal).\n");
    } else {
        pfnAdjustWindowRectExForDpi =
            (fnTypeAdjustWindowRectExForDpi)GetProcAddress(hModUser32, "AdjustWindowRectExForDpi");
        if (pfnAdjustWindowRectExForDpi) {
            DbgPrint("Found AdjustWindowRectExForDpi.\n");
        }
    }

    // Load GetDpiForWindow (which was originally named GetWindowDPI)
    pfnGetDpiForWindow =
        (fnTypeGetDpiForWindow)GetProcAddress(hModUser32, "GetDpiForWindow");
    if (pfnGetDpiForWindow) {
        DbgPrint("Found GetDpiForWindow.\n");
    } else {
        pfnGetDpiForWindow = (fnTypeGetDpiForWindow)GetProcAddress(hModUser32, "GetWindowDPI");
        if (pfnGetDpiForWindow) {
            DbgPrint("Found GetDpiForWindow. (named GetWindowDPI)\n");
        }
    }

    // Load EnableNonClientDpiScaling (or EnableChildWindowDpiMessage)
    fnEnableNCScaling =
        (fnTypeEnableNCScaling)GetProcAddress(hModUser32, "EnableNonClientDpiScaling");
    fnEnableBroadcasting =
        (fnTypeEnableBroadcasting)GetProcAddress(hModUser32, "EnableChildWindowDpiMessage");
    if (fnEnableNCScaling) {
        DbgPrint("Found EnableNonClientDpiScaling.\n");
    } else if (fnEnableBroadcasting){
        DbgPrint("Found EnableChildWindowDpiMessage.\n");
    }

    // Determine which DPI awareness to use
    BOOL bSetSystemAware = (pfnAdjustWindowRectExForDpi == NULL);

    // Call SetProcessDpiAwareness
    HRESULT ret;
    if(bSetSystemAware) {
        DbgPrint("Setting process as System DPI Aware.\n");
        ret = SetProcessDpiAwareness(PROCESS_SYSTEM_DPI_AWARE);
    } else {
        DbgPrint("Setting process as Per Monitor DPI Aware.\n");
        ret = SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);
    }
    if (ret != S_OK) {
        DbgPrintError("SetProcessDpiAwareness Failed! reason: %s\n",
            ret == E_INVALIDARG ? "Invalid Args" : "Access Denied");

        return FALSE;
    }

    // Store the awareness in gpda
    GetProcessDpiAwareness(NULL, &gpda);

    // Error if the new awareness is not the right value
    if ((gpda == PROCESS_PER_MONITOR_DPI_AWARE) != !bSetSystemAware) {
        DbgPrintError("GetProcessDpiAwareness did not return the expected DPI awareness.\n");
        return FALSE;
    }

    return TRUE;
}

BOOL EnableNonClientScalingForWindow(HWND hwnd)
{
    BOOL ret = FALSE;
    if (fnEnableNCScaling) {
        ret = fnEnableNCScaling(hwnd);

        if (ret) {
            DbgPrint("Enabled NonClient Scaling (using EnableNonClientDpiScaling)\n");
        } else {
            DbgPrintError("EnableNonClientDpiScaling failed!\n");
        }
    }
    else if (fnEnableBroadcasting) {
        ret = fnEnableBroadcasting(hwnd, TRUE);
        
        if (ret) {
            DbgPrint("Enabled NonClient Scaling (using EnableChildWindowDpiMessage)\n");
        } else {
            DbgPrintError("EnableChildWindowDpiMessage failed!\n");
        }
    }
    return ret;
}

BOOL AdjustWindowRectExForDpi_l(LPRECT lpRect, DWORD dwStyle, DWORD dwExStyle, BOOL bMenu, UINT DPI)
{
    BOOL ret = FALSE;
    if (pfnAdjustWindowRectExForDpi) {        
        ret = pfnAdjustWindowRectExForDpi(lpRect, dwStyle, bMenu, dwExStyle, DPI);
        if (!ret) {
            DbgPrintError("Error: AdjustWindowRectExForDpi failed.");
        }    
    } else {
        if (IsProcessPerMonitorDpiAware()) {
            DbgPrintError("ERROR: AdjustWindowRectEx should not be used while PM aware!\n");
        }
        ret = AdjustWindowRectEx(lpRect, dwStyle, bMenu, dwExStyle);
        if (!ret) {
            DbgPrintError("Error: AdjustWindowRectEx failed.");
        }
    }

    return ret;
}

UINT GetDpiForWindow_l(HWND hwnd)
{
    // The DPI of a System Aware window is always the system DPI,
    // and the system DPI cannot change
    if (IsProcessSystemDpiAware()) { // TODO: should check window awareness        
        static UINT SystemDPI = GetDeviceCaps(GetDC(NULL), LOGPIXELSX);
        return SystemDPI;
    }

    // Try the new API GetDpiForMonitor 
    if (pfnGetDpiForWindow) {
        return pfnGetDpiForWindow(hwnd);
    } 

    // Fallback to MonitorFromWindow and GetDpiForMonitor
    UINT dpi, garbage;
    HMONITOR hmon = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
    GetDpiForMonitor(hmon, MDT_EFFECTIVE_DPI, &dpi, &garbage);
    return dpi;    
}






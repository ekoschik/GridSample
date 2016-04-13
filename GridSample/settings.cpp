
#include "stdafx.h"
#include "GridSample.h"

BOOL bPMDpiAware;
BOOL bAllowResize = TRUE;
BOOL bSnapWindowSizeToGrid = FALSE;
BOOL bLimitWindowSizeToMonitorSize = TRUE;
BOOL bEnforceEntirelyOnMonitor = FALSE;
BOOL bHideLowPriDbg = FALSE;

BOOL InitSettingsFromArgs(int argc, char* argv[])
{
    // Return true if the arguments include '-?' or '/?'
    BOOL bHelp = FALSE;

    // Read in all of the arguments
#define TestArg(val) (_stricmp(argv[i], val) == 0)
    for (int i = 0; i < argc; i++) {
        if (TestArg("-?") || TestArg("/?")) {
            bHelp = TRUE;
        }
        if (TestArg("-NoResize") || TestArg("/NoResize")) {
            bAllowResize = FALSE;
        }
        if (TestArg("-SnapToGrid") || TestArg("/SnapToGrid")) {
            bSnapWindowSizeToGrid = TRUE;
        }
        if (TestArg("-DontLimitWindowSize") || TestArg("/DontLimitWindowSize")) {
            bLimitWindowSizeToMonitorSize = TRUE;
        }
        if (TestArg("-KeepOnMonitor") || TestArg("/KeepOnMonitor")) {
            bEnforceEntirelyOnMonitor = TRUE;
        }
        if (TestArg("-clean") || TestArg("/clean")) {
            bHideLowPriDbg = TRUE;
        }
    }

    if (bHelp) {

        DbgPrintHiPri("\nGrid Sample - Help\n\n");

        // TODO: some explanation of what on earth this is?

        DbgPrintHiPri("options:\n");
        DbgPrintHiPri("-NoResize\t - Don't allow resizing of the window\n");
        DbgPrintHiPri("-SnapToGrid\t - After beign resized, snap window to the grid size\n");
        DbgPrintHiPri("-DontLimitWindowSize - Allow any indow size (by default, limit to work area of the current monitor)\n");
        DbgPrintHiPri("-KeepOnMonitor\t - Always keep the entire window on the current monitor\n");
        DbgPrintHiPri("-clean\t - Don't print noisy/ non-important dbg output\n");

    } else {

        // Print Running Mode

        DbgPrintHiPri("Running in mode:\n");

        DbgPrintHiPri(bAllowResize ?
            "%sallow resize\n" :
            "%slocked window size\n", INDENT);
        DbgPrintHiPri(bSnapWindowSizeToGrid ?
            "%ssnap window to grid\n" :
            "%sallow free resizing of client area\n", INDENT);
        DbgPrintHiPri(bLimitWindowSizeToMonitorSize ?
            "%slimit window size to monitor size\n" :
            "%sallow any window size\n", INDENT);
        DbgPrintHiPri(bEnforceEntirelyOnMonitor ?
            "%skeep window entirely on monitor\n" :
            "%sallow window to straddle between monitors\n", INDENT);

    }
    return bHelp;
}

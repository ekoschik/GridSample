
#include "stdafx.h"
#include "GridSample.h"

BOOL bAllowResize = TRUE;
BOOL bSnapWindowSizeToGrid = FALSE;
BOOL bLimitWindowSizeToMonitorSize = TRUE;
BOOL bEnforceEntirelyOnMonitor = FALSE;

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
    }

    if (bHelp) {

        DbgPrint("\nGrid Sample - Help\n\n");

        // TODO: some explanation of what on earth this is?

        DbgPrint("options:\n");
        DbgPrint("-NoResize\t - Don't allow resizing of the window\n");
        DbgPrint("-SnapToGrid\t - After beign resized, snap window to the grid size\n");
        DbgPrint("-DontLimitWindowSize - Allow any indow size (by default, limit to work area of the current monitor)\n");
        DbgPrint("-KeepOnMonitor\t - Always keep the entire window on the current monitor\n");
        
    } else {

        // Print Running Mode

        DbgPrint("Running in mode:\n");

        DbgPrint(bAllowResize ?
            "  ---> allow resize\n" :
            "  ---> locked window size\n");
        DbgPrint(bSnapWindowSizeToGrid ? 
            "  ---> snap window to grid\n" :
            "  ---> allow free resizing of client area\n");
        DbgPrint(bLimitWindowSizeToMonitorSize ?
            "  ---> limit window size to monitor size\n" :
            "  ---> allow any window size\n");
        DbgPrint(bEnforceEntirelyOnMonitor ?
            "  ---> keep window entirely on monitor\n" :
            "  ---> allow window to straddle between monitors\n");

    }
    return bHelp;
}

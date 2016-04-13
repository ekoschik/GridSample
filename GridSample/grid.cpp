
#include "stdafx.h"
#include "GridSample.h"


BOOL bInit = FALSE;

// Block size
int blockSizeModifier = 7;
int baseBlockSize = 40;
int adjustedBlockSize = baseBlockSize;

// Grid size (in blocks)
int grid_cx = 10, grid_cy = 10;

// Drawing resources
HBRUSH hbrGrid1, hbrGrid2;
COLORREF rgbGrid1 = RGB(0, 51, 204); // blue
COLORREF rgbGrid2 = RGB(204, 153, 0); // dark yellow

int GetBlockSizeForDpi(UINT dpi)
{
    int linearScaled = ScaleToPhys(dpi, baseBlockSize);
    return linearScaled - (linearScaled % blockSizeModifier);
}

int gCurrentDpi = 96;
VOID SetGridDpi(UINT DPI)
{
    gCurrentDpi = DPI;
    adjustedBlockSize = GetBlockSizeForDpi(DPI);
    DbgPrintHiPri("Set new physical block size for DPI %i (new size: %i)\n", DPI, adjustedBlockSize);
}

VOID GetGridSize(UINT &cx, UINT &cy)
{
    cx = adjustedBlockSize * grid_cx;
    cy = adjustedBlockSize * grid_cy;
}

VOID AdjustBaseBlockSize(INT delta)
{
    // Adjust base block size, enforcing a reasonable minimum
    const static int minBlockSize = blockSizeModifier * 2;
    baseBlockSize = EnforceMinimumValue(baseBlockSize + (blockSizeModifier * delta), minBlockSize);
    adjustedBlockSize = GetBlockSizeForDpi(gCurrentDpi);

    DbgPrint("Set new logical block size: %i (adjusted: %i)\n", baseBlockSize, adjustedBlockSize);
}

VOID AdjustGridSize(INT delta)
{
    int grid_cx_prev = grid_cx,
        grid_cy_prev = grid_cy;

    // Adjust the number of rows & columns, both by delta, enforcing at least a 1x1 grid
    grid_cx = EnforceMinimumValue(grid_cx + delta, 1);
    grid_cy = EnforceMinimumValue(grid_cy + delta, 1);

    // Print the new and old grid size
    DbgPrint("Set new grid rows/cols (%i x %i), prev: (%i x %i)\n",
        grid_cx, grid_cy, grid_cx_prev, grid_cy_prev);
}

BOOL SizeGridToWindow(HWND hwnd)
{
    // TODO: make this cleaner.  essentially, a WINDOWPOSCHANGED is coming
    // in during initialization, causing SizeGridToWindow to happen before
    // SizeWindowToGrid
    if (!bInit) return FALSE;

    RECT rcClient;
    GetClientRect(hwnd, &rcClient);

    int newGridCX = RECTWIDTH(rcClient) / adjustedBlockSize;
    int newGridCY = RECTHEIGHT(rcClient) / adjustedBlockSize;

    if (newGridCX != grid_cx || newGridCY != grid_cy) {

        DbgPrint("Resizing the grid (from %i,%i to %i,%i)\n",
            grid_cx, grid_cy, newGridCX, newGridCY);

        grid_cx = newGridCX;
        grid_cy = newGridCY;

        if (bHandlingDpiChange) {
            DbgPrintError("Error: resized grid while handling a DPI change.\n");
        }

        return TRUE;
    }
    return FALSE;
}

VOID DrawGrid(HWND hwnd, HDC hdc)
{
    BOOL bColor1 = TRUE;
    RECT rcCur = { 0, 0, adjustedBlockSize, adjustedBlockSize };
    for (int y = 0; y < grid_cy; y++) {
        for (int x = 0; x < grid_cx; x++) {

            // Draw current block, and flip the color
            FillRect(hdc, &rcCur, (bColor1 ? hbrGrid1 : hbrGrid2));

            // Move rcCur to the right
            bColor1 = !bColor1;
            rcCur.left = rcCur.right;
            rcCur.right += adjustedBlockSize;
        }

        // Reset rcCur to next row, and reset the color to the next row
        rcCur.left = 0;
        rcCur.right = adjustedBlockSize;
        rcCur.top = rcCur.bottom;
        rcCur.bottom += adjustedBlockSize;
        bColor1 = (y % 2 == 1);
    }
}

VOID InitGrid(HWND hwnd)
{
    bInit = TRUE;
    hbrGrid1 = CreateSolidBrush(rgbGrid1);
    hbrGrid2 = CreateSolidBrush(rgbGrid2);

    SetGridDpi(GetDpiForWindow_l(hwnd));
}

#include "stdafx.h"
#include "GridSample.h"

// Block size
int blockSizeModifier = 15;
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

VOID SetGridDpi(UINT DPI)
{
    adjustedBlockSize = GetBlockSizeForDpi(DPI);
    DbgPrint("Set new block size: %i\n", adjustedBlockSize);
}

VOID InitGrid(HWND hwnd)
{
    hbrGrid1 = CreateSolidBrush(rgbGrid1);
    hbrGrid2 = CreateSolidBrush(rgbGrid2);

    SetGridDpi(GetDpiForWindow_l(hwnd));
}

VOID GetGridSize(UINT &cx, UINT &cy)
{
    cx = adjustedBlockSize * grid_cx;
    cy = adjustedBlockSize * grid_cy;
}

VOID ResizeGridWindowResize(RECT rcClient)
{
    int newGridCX = RECTWIDTH(rcClient) / adjustedBlockSize;
    int newGridCY = RECTHEIGHT(rcClient) / adjustedBlockSize;

    if (newGridCX != grid_cx || newGridCY != grid_cy) {

        DbgPrint("Resizing the grid (from %i,%i to %i,%i)\n",
            grid_cx, grid_cy, newGridCX, newGridCY);

        grid_cx = newGridCX;
        grid_cy = newGridCY;

        if (bHandlingDpiChange) {
            DbgPrintError("Error: Should not have to resize grid while handling a DPI change\n");
        }
    }
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

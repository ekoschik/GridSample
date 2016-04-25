
#include "stdafx.h"
#include "GridSample.h"


VOID Grid::RecalcAdjustedBlockSize()
{
    int linearScaled = (int)(baseBlockSize * scale);
    adjustedBlockSize = linearScaled - (linearScaled % blockSizeModifier);
}

VOID Grid::SetScale(float newScale)
{
    scale = newScale;
    RecalcAdjustedBlockSize();

    DbgPrint(
        "Set new grid scale factor (%.2fx), setting adjusted block size to %i.\n",
        scale, adjustedBlockSize);
}

VOID Grid::GetSize(UINT &cx, UINT &cy)
{
    cx = adjustedBlockSize * grid_cx;
    cy = adjustedBlockSize * grid_cy;
}

VOID Grid::AdjustBlockSize(INT delta)
{
    // Adjust base block size, enforcing a reasonable minimum
    const static int minBlockSize = blockSizeModifier * 2;
    baseBlockSize = EnforceMinimumValue(baseBlockSize + (blockSizeModifier * delta), minBlockSize);
    RecalcAdjustedBlockSize();

    DbgPrint("Set new logical block size: %i (adjusted: %i)\n", baseBlockSize, adjustedBlockSize);
}

VOID Grid::AdjustGridSize(INT delta)
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

BOOL Grid::SizeToRect(RECT rc)
{
    int newGridCX = RECTWIDTH(rc) / adjustedBlockSize;
    int newGridCY = RECTHEIGHT(rc) / adjustedBlockSize;

    if (newGridCX != grid_cx || newGridCY != grid_cy) {

        DbgPrint("Resizing the grid (from %i,%i to %i,%i)\n",
            grid_cx, grid_cy, newGridCX, newGridCY);

        grid_cx = newGridCX;
        grid_cy = newGridCY;
        return TRUE;
    }
    return FALSE;
}

VOID Grid::Draw(HDC hdc)
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

VOID Grid::Init(INT cx, INT cy, INT blocksize)
{
    baseBlockSize = blocksize;
    grid_cx = cx;
    grid_cy = cy;

    hbrGrid1 = CreateSolidBrush(rgbGrid1);
    hbrGrid2 = CreateSolidBrush(rgbGrid2);
}
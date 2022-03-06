#include "PixelRGB.h"

PixelRGB** PixelRGB::CreateContiguous2DPixmap(int rows, int cols, bool initValues)
{
    // Avoid some invalid array exceptions.
    if (rows <= 0 || cols <= 0) return nullptr;

    // Allocate pixmap in a way such that memory is contiguous and therefore
    // accessed much quicker than a normal 2d array.
    PixelRGB** pixmap;
    pixmap = new PixelRGB * [rows];
    pixmap[0] = new PixelRGB[rows * cols];

    for (int i = 1; i < rows; i++)
        pixmap[i] = pixmap[i - 1] + cols;

    if (initValues)
    {
        for (int row = 0; row < rows; row++)
            for (int col = 0; col < cols; col++)
            {
                // Init with normal map coding for pointing right at viewer.
                pixmap[row][col].r = 128;
                pixmap[row][col].g = 128;
                pixmap[row][col].b = 255;
            }
    }

    return pixmap;
}

void PixelRGB::DeleteContiguous2DPixmap(PixelRGB**& toDeletePixmap)
{
	delete[] toDeletePixmap[0];
	delete[] toDeletePixmap;
}

void PixelRGB::CopyData(const PixelRGB* src, PixelRGB* dest)
{
    dest->r = src->r;
    dest->g = src->g;
    dest->b = src->b;
}

PixelRGB PixelRGB::MakePixel(unsigned char r, unsigned char g, unsigned char b)
{
    PixelRGB newPix;
    newPix.r = r;
    newPix.g = g;
    newPix.b = b;
    
    return newPix;
}

bool PixelRGB::Equals(const PixelRGB* a, const PixelRGB* b)
{
    return (a->r == b->r) && (a->g == b->g) && (a->b == b->b);
}

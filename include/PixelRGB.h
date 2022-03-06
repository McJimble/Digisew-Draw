#ifndef PIXELRGB_H
#define PIXELRGB_H

/*
 *	Defines an RGB pixel using unsigned chars.
 */
struct PixelRGB
{
	unsigned char r;
	unsigned char g;
	unsigned char b;

	/*
	 *	Returns a pointer to PixelRGB pointer that can be interpreted + accessed as
	 *  a traditional 2D array, but is fully contiguous in memory such that it can be
	 *  casted to (void*) and passed into SDL or OpenGL rendering functions.
	 * 
	 *	Returns nullptr if given rows/cols are invalid (i.e. less than or equal to zero).
	 */
	static PixelRGB** CreateContiguous2DPixmap(int rows, int cols, bool initValues = true);

	/*
	 *	Deletes a given 2D contiguous array of PixelRGBs.
	 */
	static void DeleteContiguous2DPixmap(PixelRGB**& toDeletePixmap);

	/*
	 *	Acts as a memcpy for PixelRGBs.
	 */
	static void CopyData(const PixelRGB* src, PixelRGB* dest);

	/*
	 *	Static constructor for a pixel rgb. Again, had to do this to keep the memory
	 *  contiguity of this data type.
	 */
	static PixelRGB MakePixel(unsigned char r, unsigned char g, unsigned char b);

	/*
	 *	Determine if two pixels are equal in all values (rgb) 
	 */
	static bool Equals(const PixelRGB* a, const PixelRGB* b);
};

#endif
#pragma once

#include "Image.h"
#include "PixelRGB.h"

#include <memory>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

class StitchResult
{
public:

	StitchResult(int w, int h, const PixelRGB**& normalMap);
	~StitchResult();

	/*
	 *	Uses given map/image information and computes stitch information
	 *  using the legacy code and algorithms. 
	 *
	 *	A new window and renderer will be created
	 *  and displayed on the screen upon calling this function, which can closed
	 *  at any time safely.
	 * 
	 *  Returns: True if stitch computed and window opened successfully (if requested); otherwise false.
	 */
	bool CreateStitches(bool createWindow = true);

private:

	static int resultID;

	std::unique_ptr<Image> stitchImg;
	std::unique_ptr<Image> densityMapImg;
	std::unique_ptr<Image> normalMapImg;

	SDL_Renderer* renderer;
	SDL_Window* window;

	const float constantDensity = 0.1f;	// Hardcoded 0.1 density across the board
	const int subgridSize = 10;			// Hardcoded subgrid size of 10 for now.
};
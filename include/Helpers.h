#ifndef HELPERS_H
#define HELPERS_H

#include <SDL2/SDL.h>

#include "PixelRGB.h"
#include "Vector2D.h"

class Helpers
{

public:

	/*
	*	Linearly interpoates between two PixelRGBs in JUST RGB space, not HSV.
	*	Therefore, the lerp isn't perfectly accurate, but gets the job done for now.
	*/
	static PixelRGB LerpColorRGB(const PixelRGB& p1, const PixelRGB& p2, float t);

	/*
	 *	Outputs a new PixelRGB value based on an SDL color.
	 */
	static PixelRGB SDLColorToPixel(const SDL_Color& col);

	/*
	 *	Outputs a new SDL_Color value based on a PixelRGB  
	 */
	static SDL_Color PixelToSDLColor(const PixelRGB& pix);

	/*
	 *	Just gives an SDL_Color representing the normal map-coded color
	 *  for normals pointing directly at the user.
	 *	Can alternatively get pixel color directly with PixelRGB variant.
	 */
	static void NormalMapDefaultColor(SDL_Color* ret);
	static void NormalMapDefaultColor(PixelRGB* ret);
	static PixelRGB NormalMapDefaultColor();

	/*
	 *	Sets the RGB values in a given SDL_Color to corresponding ones of
	 *  the given hsv values.
	 */
	static void HSVtoRGB(float h, float s, float v, PixelRGB* p);

	/*
	 *	Generates HSV values based on the given SDL_Color.
	 *  Returns the HSV values by reference.
	 */
	static void RGBtoHSV(float& h, float& s, float& v, const PixelRGB* p);

	/*
	 *	Converts a given vector to a normal map-coded color. It assumes
	 *  the line drawn acts as an straight extrustion from the sampled image,
	 *  meaning the blue channel will be given a value of 128 by default due
	 *  to the normal of a "wall" perpendicular to the image plane has a z-value of zero.
	 */
	static void Vector2DtoNormalColor(const Vector2D& vec, PixelRGB* ret);
	static void Vector2DtoNormalColor(const Vector2D& vec, SDL_Color* ret);

	/*
	 *	Same as original method, but restricts the color to half of the color
	 *  range allowed by normal maps. Can be used to avoid discrepancy between'
	 *  what direction a line was drawn.
	 */
	static void Vector2DtoNormalColorHalf(const Vector2D& vec, PixelRGB* ret, bool positivePolarity = true);
	static void Vector2DtoNormalColorHalf(const Vector2D& vec, SDL_Color* ret, bool positivePolarity = true);

	/*
	 *	Returns a vector that represents the normal map direction given by
	 *  the vales of r and g (ignoring the blue channel/z-axis).
	 */
	static Vector2D HalfNormalColorToDirection(Uint8 r, Uint8 g, bool positivePolarity = true);

	/*
	 *  Naive return value is unclamped and doesn't account for edge cases
	 *  like max == min, in which case NaN will return due to divide by 0. 
	 */
	static float InverseLerp(float min, float max, float val);

	/*
	 *	Returns a naively lerped value between min and max. Doesn't account
	 *  for edge cases besides float precision error (uses slightly slower method)
	 */
	static float Lerp(float min, float max, float t);

	/*
	 *  Linearly interpolates between two colors, and returns it into ret.
	 */
	static void LerpColor(const PixelRGB& c1, const PixelRGB& c2, float t, PixelRGB* ret);

	/*
	 *	Computes barycentric coordinates of a point within a triangle; assumes the z-axis does
	 *  not exist in order to have a more efficient algorithm.
	 */
	static void ComputeBarycentricCoordinates(const Vector2D& p, const Vector2D& a, const Vector2D& b, const Vector2D& c, float& u, float& v, float& w);

	/*
	 *	Determines whether point P interesects with the triangle formed by a, b, and c,
	 *	using point a as the origin. Algorithm is from: https://mathworld.wolfram.com/TriangleInterior.html
	 */
	static bool PointTriangleIntersection(const Vector2D& p, const Vector2D& aOrigin, const Vector2D& b, const Vector2D& c);

	/*
	 *	If val is greater than max or less than min, value returned
	 *  returned is min or min respectively; effectively ensuring val
	 *  always lies between min and max.
	 */
	static float Clamp(float val, float min, float max);

	/*
	 *  Uses SDL_DrawPoint function to draw an unfilled circle. Doesn't account for render color.
	 *  Pulled directly from here: https://stackoverflow.com/a/48291620
	 */
	static void SDL_DrawCircle(SDL_Renderer* renderer, int32_t centreX, int32_t centreY, int32_t radius);
	
};

#endif
#pragma once
#include "Vector2D.h"
#include "PixelRGB.h"

#include <memory>

#include <SDL2/SDL.h>

class VoronoiPoint
{
public:

	static int pixelRadius;				// Width of voronoi point render in pixels.

	VoronoiPoint(const Vector2D& position, const PixelRGB& normalEncoding, int zone = 0);
	VoronoiPoint(const Vector2D& position, const SDL_Color& normalEncoding, int zone = 0);
	~VoronoiPoint();

	void RenderPoint(SDL_Renderer* rend);

	const int Get_VoronoiZone() const;
	const Vector2D& Get_Position() const;
	const PixelRGB& Get_NormalEncoding() const;

	void Set_VoronoiZone(int zone);	// In case we want to change zone image at runtime.
	void Set_NormalEncoding(const SDL_Color& normalEncoding);

private:

	int zoneIndex;						// Zone this point is inside of.

	Vector2D position;					// Position of point on the screen.
	PixelRGB normalEncoding;			// Normal encoded color provided for this point.
};

// Comparator for sorting by x position of the point (curretly unused)
struct VoronoiPointCmp
{
	bool operator() (const std::shared_ptr<VoronoiPoint>& lhs, const std::shared_ptr<VoronoiPoint>& rhs) const
	{
		return lhs->Get_Position()[0] < rhs->Get_Position()[0];
	}
};


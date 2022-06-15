#pragma once
#include "Vector2D.h"
#include "PixelRGB.h"
#include "IntersectionNode.h"

#include <memory>
#include <vector>

#include <SDL2/SDL.h>

class VoronoiPoint
{
public:

	static int pixelRadius;				// Width of voronoi point render in pixels.

	VoronoiPoint(const Vector2D& position, const PixelRGB& normalEncoding, int zone = 0);
	VoronoiPoint(const Vector2D& position, const SDL_Color& normalEncoding, int zone = 0);

	void RenderPoint(SDL_Renderer* rend);
	void RenderPoint(SDL_Renderer* rend, const SDL_Color& overrideColor);
	void RenderFormedTriangles(SDL_Renderer* rend);

	void AddNode(const std::shared_ptr<IntersectionNode>& node);
	bool RemoveNode(int nodeID);
	void ClearNodes();
	void FlipPolarity();

	const std::vector<std::shared_ptr<IntersectionNode>>& Get_NeighboringNodes() const;
	const int Get_ID() const;
	const int Get_VoronoiZone() const;
	const Vector2D& Get_Position() const;
	const Uint8 Get_VoronoiDensity() const;
	const PixelRGB& Get_NormalEncoding() const;
	bool Get_PolaritySwapped() const;

	void Set_Polarity(bool pol);
	void Set_VoronoiZone(int zone);	// In case we want to change zone image at runtime.
	void Set_VoronoiDensity(const Uint8 newDens);
	void Set_NormalEncoding(const SDL_Color& normalEncoding);
	void Set_RenderColor(const SDL_Color& renderColor);
	void Set_Position(const Vector2D& newPos);

private:

	static int nextID;

	std::vector<std::shared_ptr<IntersectionNode>> neighboringNodes;

	bool positivePolarity;				// True = Uses Positve X side of normal map color wheel only; False = Uses negative X only.
	int id;								// ID of this object given at creation.
	int zoneIndex;						// Zone this point is inside of.
	Vector2D position;					// Position of point on the screen.
	SDL_Color renderColor;				// Color of the dot itself
	
	Uint8 voronoiDensity;				// Density of stitches around this point; starts at 128.
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


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
	void RenderFormedTriangles(SDL_Renderer* rend);

	void AddNode(IntersectionNode* node);
	bool RemoveNode(IntersectionNode* node);

	const std::vector<IntersectionNode*>& Get_NeighboringNodes() const;
	const int Get_ID() const;
	const int Get_VoronoiZone() const;
	const Vector2D& Get_Position() const;
	const PixelRGB& Get_NormalEncoding() const;

	void Set_VoronoiZone(int zone);	// In case we want to change zone image at runtime.
	void Set_NormalEncoding(const SDL_Color& normalEncoding);

private:

	static int nextID;

	std::vector<IntersectionNode*> neighboringNodes;

	int id;								// ID of this object given at creation.
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


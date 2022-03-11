#pragma once
#include "Vector2D.h"
#include "PixelRGB.h"

#include <vector>
#include <memory>
#include <utility>

#include <SDL2/SDL.h>

class VoronoiPoint;

/*
 *	Acts as a simple container to describe areas where edges of the voronoi map
 *	intersect. Can be anywhere on the screen, including the edges and corners.
 *  Therefore, the position must be manually passed in based on the specific use case/location.
 */
class IntersectionNode
{
public:

	static int maxPoints;

	IntersectionNode(const Vector2D& setPosition, std::vector<VoronoiPoint*> intersectingPoints);

	// Forces node to update its color based on current values of connected points
	void UpdateColor();

	// Render where the intersection node is (currently just for debugging purposes.
	void RenderNode(SDL_Renderer* rend);

	// If a new point was placed closer than the distance the node is from its
	// equidistant voronoi points, then it should be dissolved and deleted.
	bool CheckForShouldDissolve(VoronoiPoint* newPoint);

	int Get_ID() const;
	const Vector2D& Get_Position() const;
	const PixelRGB& Get_AverageColor() const;

private:

	static int nextID;

	// List of points this connects, paired with their distance from this node.
	std::vector<std::pair<float, std::shared_ptr<VoronoiPoint>>> intersectingPoints;

	int identifier;
	Vector2D position;
	PixelRGB averageColor;
	float pointsDistance;
};

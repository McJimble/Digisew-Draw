#pragma once
#include <vector>

#include "VoronoiPoint.h"

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
	~IntersectionNode();

	// If a new point was placed closer than the distance the node is from its
	// equidistant voronoi points, then it should be dissolved and deleted.
	bool CheckForShouldDissolve(VoronoiPoint* newPoint);

	const Vector2D& Get_Position();
	const SDL_Color& Get_AverageColor();
	

private:

	// List of points this connects, paired with their distance from this node.
	std::vector<std::pair<float, std::shared_ptr<VoronoiPoint>>> intersectingPoints;

	Vector2D position;
	SDL_Color averageColor;
	float pointsDistance;
};
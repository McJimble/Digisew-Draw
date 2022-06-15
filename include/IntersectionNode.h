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

	IntersectionNode(const Vector2D& setPosition, const std::vector<VoronoiPoint*>& intersectingPoints, int zone);
	~IntersectionNode();

	// Forces node to update its color based on current values of connected points
	void UpdateColor();

	// Foroces node to update DENSITY based on connected positions.
	void UpdateDensity();

	// Render where the intersection node is (currently just for debugging purposes.
	void RenderNode(SDL_Renderer* rend);

	int Get_ID() const;
	int Get_VoronoiZone() const;
	const Vector2D& Get_Position() const;
	const float Get_AverageDensity() const;
	const PixelRGB& Get_AverageColor() const;
	const std::vector<VoronoiPoint*>& Get_IntersectingPoints() const;

	bool EnvelopesSamePoints(const IntersectionNode& other) const;

private:

	static int nextID;

	// List of points this connects.
	std::vector<VoronoiPoint*> intersectingPoints;

	int identifier;
	int voronoiZone;
	Vector2D position;

	float averageDensity;
	PixelRGB averageColor;
};

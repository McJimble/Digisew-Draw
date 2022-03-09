#ifndef DYNAMICPIXEL_H
#define DYNAMICPIXEL_H

#include <vector>
#include <memory>

#include <SDL2/SDL.h>

#include "IntersectionNode.h"
#include "VoronoiPoint.h"
#include "PixelRGB.h"
#include "Helpers.h"

/*
 *	Using a list of colors that different sketch lines are attempting to
 *  set a pixel as, 
 */
class DynamicColor
{

public:

	DynamicColor(PixelRGB* affectedPixel, const Vector2D& position);
	~DynamicColor();

	/*
	 *	Updates pixel color based on distance between two closest voronoi points
	 *  and their respective colors.
	 */
	void UpdatePixel();

	/*
	 *	Attempts to update the currently stored minimum point values; if the given
	 *  points distance from this pixel is less than either, then it updated and
	 *  returns true. Otherwise, does nothing and returns false.
	 */
	bool TryAddMinPoint(const std::shared_ptr<VoronoiPoint>& newPoint);

	/*
	 *	Deprecated method of updating the color directly with some interpolation.
	 */
	void UpdatePixelInterp(const PixelRGB* newColor, float t);

	/*
	 *	Sets the voronoi zone of this color/pixel. 
	 */
	void SetVoronoiZone(int newZoneID);

	float Get_VornoiDensity();
	const PixelRGB* Get_AffectedPixel();
	const Vector2D& Get_PixelPosition();
	VoronoiPoint* Get_MinPoint();

private:

	// Value between 0 - 1 that determines the voronoi density that creates black
	// pixels. Should ideally be really close one to avoid distorted edges.
	static float edgeThreshold;

	// 2 intersection nodes that form triangle it's within.
	std::pair<std::shared_ptr<IntersectionNode>, std::shared_ptr<IntersectionNode>> triangulationNodes;	
	Vector2D barycentricCoords;		// Barycentric coordinates in triangle with nodes. MinPt below is the origin.

	std::shared_ptr<VoronoiPoint> minPt;		// Closest voronoi point to this pixel
	std::shared_ptr<VoronoiPoint> secondMinPt;	// 2nd closest point to this pixel; used for determining density.
	float minPtDistance = FLT_MAX;
	float secondMinPtDistance = FLT_MAX;

	// Cuerent implementation gets this by determining the diff. between the closest and
	// second closest voronoi points to this pixel. Equidistant points should have density of 1
	// as the distances are (nearly) equivalent and should form an edges.
	float voronoiDensity;

	// Zone this pixel inside of. Only voronoi points that match this zone can be added.
	int voronoiZone;

	// Reference to RGB pixel this will modify.
	PixelRGB* affectedPixel;

	// Position of pixel on screen (float converted)
	Vector2D pixPosition;

	// Changed the affected pixel to a value between black and white
	// based on the voronoi density.
	void DensityToLuminosity();

	// Change the affect pixel's value (in hsv space) based on the voronoi
	// density that is currently applied.
	void DensityToColor();
};


#endif
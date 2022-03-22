#ifndef DYNAMICPIXEL_H
#define DYNAMICPIXEL_H

#include <vector>
#include <memory>

#include <SDL2/SDL.h>

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

	/*
	 *	Returns true if conditions were met that allowed these to all be set. 
	 */
	bool Set_TriangulationNodes(IntersectionNode* a, IntersectionNode* b, const Vector2D& origin);

	bool ContainsNode(IntersectionNode* node);

	float Get_VornoiDensity();
	const PixelRGB* Get_AffectedPixel();
	const Vector2D& Get_PixelPosition();
	VoronoiPoint* Get_MinPoint();
	
private:

	// Value between 0 - 1 that determines the voronoi density that creates black
	// pixels. Should ideally be really close one to avoid distorted edges.
	static float edgeThreshold;

	// 2 intersection nodes that form triangle it's within.
	IntersectionNode* triNodeA;
	IntersectionNode* triNodeB;
	float baryU, baryV, baryW;	// Barycentric coordinates used for interpolating color.

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

	// OUTDATED
	// Change the affect pixel's value (in hsv space) based on the voronoi
	// density that is currently applied.
	void DensityToColor();

	// Change the affected pixel's value based on the current barycentric coordinates
	// the pixel has within a triangle formed by voronoi points / nodes.
	void BarycentricToColor();
};


#endif
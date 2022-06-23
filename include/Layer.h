#pragma once

#include <vector>
#include <unordered_map>

#include "DynamicColor.h"

class Layer
{
public:

	// Using this will make pixels dynamic and EDITABLE with voronoi points.
	Layer(int sizeX, int sizeY, int zone);

	// Using this will load pixels with data from an image and the map is NOT editable.
	Layer(const std::string& normalName, const std::string& densityName, int sizeX, int size, int zone);
	~Layer();

	void AddVoronoiPoint(const std::shared_ptr<VoronoiPoint>& newPt, bool updateBarycentric = true);

	/*
	 *	Removes a node from this layer. Doesn't rebuild though, so use if you are planning to do that. 
	 */
	void RemovePoint(const std::shared_ptr<VoronoiPoint>& toRemove);

	void RecolorSelectedPoints(std::unordered_map<int, std::shared_ptr<VoronoiPoint>>& selectedPoints);

	/*
	 *	Forces layer to update all its pixels. Slow operation! 
	 */
	void UpdateLayerAll(bool barycentric);

	/*
	 *	Clears pixelsToUpdate array. 
	 */
	void CancelUpdate();

	/*
	 *	Clears all voronoi data.
	 */
	void ClearData();

	/*
	 *	Only updates pixels in the pixelsToUpdate array. 
	 */
	void UpdateQueuedPixels();

	/*
	 *	Renders voronoi points and (if debug is on) cell borders/intersections. 
	 */
	void RenderLayer(SDL_Renderer* rend, bool showDebug);

	/*
	 *	Returns reference to the exact pixel that is finally affected by the state of the
	 *  layer currently. This allows us to track specific pixel values to construct
	 *  the final texture outside of this scope.
	 * 
	 *  FOR EFFICIENCY SAKE this doesn't bound check so don't pass in 
	 */
	void Set_PixelRefs(int x, int y, PixelRGB* normalPix, PixelRGB* densityPix);

	bool Get_IsEditable()
	{
		return editable;
	}

private:
	
	bool editable = true;
	int sizeX, sizeY;	// Caches layer size; should be same as main window.

	PixelRGB** rawNormalData;
	PixelRGB** rawDensityData;
	std::vector<std::vector<DynamicColor*>> normalMap;					// DynamicColor instances for this layer; covers whole screen although all may not be seen.
	std::vector<DynamicColor*> pixelsToUpdate;							// Pixels to update next loop.

	std::unordered_map<int, std::shared_ptr<VoronoiPoint>> ownedPoints;	// Pts created on this layer; still globably accessible in main SketchProgram.
	std::unordered_map<int, std::shared_ptr<IntersectionNode>> createdNodes; // Interesection nodes on this layer.

	/*
	 *	Helper function for detecting intersection points between voronoi cells.
	 */
	void CheckForIntersections(const std::vector<DynamicColor*>& toCheck);

	/*
	 *	Updates the barycentric coordinates for all pixels in the list.
	 */
	void BarycentricUpdate(const std::vector<DynamicColor*>& toUpdate);
};
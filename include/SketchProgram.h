#ifndef SKETCHPROGRAM_H
#define SKETCHPROGRAM_H

#include <iostream>
#include <vector>
#include <set>
#include <memory>
#include <unordered_map>

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <SDL2/SDL_image.h>
#include <GL/gl.h>

#include "PixelRGB.h"
#include "SketchLine.h"
#include "Helpers.h"
#include "DynamicColor.h"
#include "VectorField.h"
#include "StitchResult.h"
#include "Layer.h"

#undef main

#define WINDOW_NAME "Digisew-Draw"

/*
 *	Main engine/driver for the sketch program. Handles main functionality such as
 *	displaying the window, main program loop, etc.
 */
class SketchProgram
{

public:

	/*
	 *	Initializes window, sdl, opengl, etc.
	 */
	void Initialize(int argc = 0, char** argv = nullptr);

	/*
	 *	Begins main program loop. Call Initialize() and then this function,
	 *  and the program should run as intended.
	 */
	void MainLoop();

	/*
	 *	Exits program.
	 */
	void Quit();

	/*
	 *	Saves current color map to a file; exludes extra vector fields and vornoi cell edges
	 *  
	 *	filename = name of image that will be saved
	 *  
	 different filetypes currently supported:
	 *		bmp
	 *      png
	 *		jpg (SDL_Image.h must be properly linked libjpeg, which may be weird to configure)
	 */
	void SaveColorMap(std::string& filename, int winID = 0);

private:

	std::vector<std::unique_ptr<StitchResult>> stitchResults;

	SDL_DisplayMode displayConfig;	// Display configurations (screen size, refresh rate, etc.)
	SDL_Window* window;				// Window for SDL
	SDL_Renderer* renderer;			// Renderer for SDL
	int screenWidth, screenHeight;	// Size of window

	PixelRGB** normalMapPixels;													// 2D array of the actual pixels displayed on texture.
	PixelRGB** densityMapPixels;
	SDL_Texture* normalMapTexture;												// Texture being displayed/changed in background.
	std::vector<std::unique_ptr<Layer>> layers;
	
	std::vector<std::vector<int>> voronoiZonesByPixel;							// Voronoi zone for each pixel on screen.
	std::vector<std::shared_ptr<SketchLine>> sketchLines;						// Lines drawn by the user; not normally shown but are stored for debugging.

	// All voronoi points that have been placed. This link suggests sorting them by x-distance
	// if needed to iterate through them, so that may be added later.
	// https://www.codeproject.com/Articles/882739/Simple-Approach-to-Voronoi-Diagrams
	std::unordered_map<int, std::shared_ptr<VoronoiPoint>> voronoiPoints;					// All voronoi points created by the user.
	int newestPointID;

	std::unique_ptr<VectorField> mainVecField;
	int fieldX, fieldY;
	int fieldPadding;

	// Too many bools for determining program state; this could prob. use refactor, sorry
	bool isRunning = true;					// Is main program loop currently running?
	bool leftMouseDownLastFrame = false;	// Was left mouse held down last frame
	bool rightMouseDownLastFrame = false;	// Was right mouse held down last frame
	bool middleMouseDownLastFrame = false;	// Was middle mouse held down last frame
	bool debugDisplay = false;				// Displays visuals for extra elements
	bool enableDeletion = false;			// Can nodes be deleted with mouse instead
	bool enablePersistentSelection = false;	// When true, selected points are not removed from list if not selected.
	bool pointPositionsDirty = false;		// Have point positions been moved and should therefore refresh the map?
	
	bool densityMode = false;				// Is density mode currently active (only density changes, not colors)?

	// Parameters read from starting parameters file:
	int pWidth = 0;
	int pHeight = 0;
	double pVectorFieldDensityFac = 0.0f;
	std::string defaultNormalMap = "";
	std::string defaultDensityMap = "";
	std::string zoneMapName = "";

	Vector2D mousePos;						// Cached position of mouse in screen space.
	Vector2D prevMousePos;					// Mouse position of previous frame; used to displace things with mouse movement.
	SDL_Rect selectRect;
	std::unordered_map<int, std::shared_ptr<VoronoiPoint>> selectedPoints;	// Box-selected points using right click
	Vector2D initSelectionPoint = Vector2D(-1.0f, -1.0f);					// Place where right mouse was first clicked to select.

	SDL_Color black = { 0, 0, 0, 255 };
	SDL_Color white = { 255, 255, 255, 255 };
	SDL_Color red = { 255, 0, 0, 255 };

	/*
	 *	For performing a complete program loop that polls input, updates, and renders in real time.  
	 */
	void PollEvents();
	void Update();
	void Render();

	void ReadParameters(const char* paramFile);

	/*
	 *	Using stb_image, reads raw pixel from an image file and stores
	 *  in a custom structure we use to determine voronoi zones.
	 *  Calling will reset and reallocate the voronoi map.
	 */
	void ParseZoneMap(const std::string& filename = "");

	/*
	 *	Creates a default mesh containing 10 rows and columns of evently spaced voronoiPoints.
	 */
	void LoadDefaultMesh();

	/*
	 *	Allocates a texture to text parameter based on data provided by raw
	 *  pixel bytes. Currently just assumes pixels contain 1 byte per color channel (Uint8).
	 */
	void CreateTextureFromPixelData(SDL_Texture*& text, void* pixels, int w, int h, int channels);

	/*
	 *	Places a new voronoi point and updates the displayed map
	 */
	void EmplaceVoronoiPoint(std::shared_ptr<VoronoiPoint>& editpt, bool updateAffectedBarycentric = true);

	/*
	 *	Draw line, emplace point when mouse down
	 *	
	 *	placePoint: will the sketch line's creation also place a voronoi map simultaneously.
	 *	
	 *	returns: reference to the sketch line that was either newly created OR
	 *  is currently being editted because of current state of the program.
	 */
	SketchLine* DrawSketchLine(bool placePoint);

	/*
	 *	Does the SDL_Rect overlap this point?
	 */
	bool PointRectOverlap(const SDL_Rect& aabb, const Vector2D& pt);

	/*
	 *	Moves selected points with left mouse movement; recompute map on left mouse up.
	 */
	void MoveSelectedPoints();

	/*
	 *	Controls logic of recoloring all points currently selected and updating pixels
	 *
	 *	followLine: Sketch line to color pixels of
	 */
	void RecolorSelectedPoints(SketchLine* followLine);

	/*
	 *	Generates the entire map based on current voronoi points that already have
	 *	vertices/nodes determined. No optmization at all, just n^3 implementation that checks
	 *  possible points with each pixel.
	 */
	void RebuildMapNaive();

	/*
	 *	Deletes all nodes currently selected, if any. 
	 */
	void DeleteSelectedPoints();

	/*
	 *	Uses pieces of original stitch generation code to open a new window
	 *	containing final stitch output using the normal map at the state it's in
	 */
	void CreateStitchDiagram();

	/* Quick helper functions to determine if use is allowed to edit voronoi in certain mode*/

	int Get_ScreenHeight();
	int Get_ScreenWidth();
};

#endif
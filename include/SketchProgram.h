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

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

#undef main

// Fixing window name + dimensions for now to make start code simpler.
// Can change later if needed.
#define WINDOW_NAME "Digisew-Draw"
#define DEF_SCREEN_WIDTH 1000
#define DEF_SCREEN_HEIGHT 1000

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
	void Initialize();

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
	 *		jpg
	 */
	void SaveColorMap(std::string& filename);

private:

	SDL_DisplayMode displayConfig;	// Display configurations (screen size, refresh rate, etc.)
	SDL_Window* window;				// Window for SDL
	SDL_Renderer* renderer;			// Renderer for SDL
	int screenWidth, screenHeight;	// Size of window

	PixelRGB** normalMapPixels;													// Actual pixels displayed on texture.
	SDL_Texture* normalMapTexture;												// Texture being displayed/changed in background.
	std::unordered_map<int, std::shared_ptr<IntersectionNode>> createdNodes;	// Intersection nodees that have been created thus far (where edges connect)
	std::vector<std::vector<int>> voronoiZonesByPixel;							// Voronoi zone for each pixel on screen.
	std::vector<std::vector<std::shared_ptr<DynamicColor>>> normalMapColors;	// Colors being sent by normal map sketches
	std::vector<std::shared_ptr<DynamicColor>> pixelsToUpdate;					// Pixels that will be updated this frame.
	std::vector<std::shared_ptr<SketchLine>> sketchLines;						// Lines drawn by the user; not normally shown but are stored for debugging.

	// Voronoi points that have been placed. This link suggests sorting them by x-distance
	// if needed to iterate through them, so that may be added later.
	// https://www.codeproject.com/Articles/882739/Simple-Approach-to-Voronoi-Diagrams
	std::vector<std::shared_ptr<VoronoiPoint>> voronoiPoints;					// All voronoi points created by the user.

	std::shared_ptr<VectorField> mainVecField;
	int fieldX, fieldY;
	int fieldPadding;

	bool mouseDownLastFrame = false;	// Was left mouse held down last frame
	bool isRunning = true;				// Is main program loop currently running?
	bool debugDisplay = false;			// Displays visuals for extra elements
	bool enableDeletion = false;		// Can nodes be deleted with mouse instead
	int deletionRadius = 15;
	std::shared_ptr<VoronoiPoint> toDelete;

	SDL_Color black = { 0, 0, 0, 255 };
	SDL_Color white = { 255, 255, 255, 0 };

	void PollEvents();
	void Update();
	void Render();

	/*
	 *	Using stb_image, reads raw pixel from an image file and stores
	 *  in a custom structure we use to determine voronoi zones.
	 *  Calling will reset and reallocate the voronoi map.
	 */
	void ParseZoneMap(const std::string& filename = "");

	/*
	 *	Allocates a texture to text parameter based on data provided by raw
	 *  pixel bytes. Currently just assumes pixels contain 1 byte per color channel (Uint8).
	 */
	void CreateTextureFromPixelData(SDL_Texture*& text, void* pixels, int w, int h, int channels);

	/*
	 *	Places the currently created sketch line and modifies the pixels
	 *  with the area it covers. Also updates the current texture
	 *  OUTDATED, CREATES CIRCLE ON TOP OF TEXTURE
	 */
	void EmplaceSketchLine(SketchLine* editLine);

	/*
	 *	Places a new voronoi point and updates the displayed map
	 */
	void EmplaceVoronoiPoint(SketchLine* editline);

	int Get_ScreenHeight();
	int Get_ScreenWidth();
};

#endif
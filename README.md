Author: James Robertson
Date: 6/23/2022

# Digisew-Draw

An unnamed sketch-based program that is used to draw images specifically tailored for research purposes at Clemson University.
Created using C++ and the SDL2 library for simple rendering.

Currently in active development.

# Dependencies
 **Header only, included in source already:**
  - stb_image.h https://github.com/nothings/stb/blob/master/stb_image.h
  - glm https://github.com/g-truc/glm
  
 **Must install yourself:**
  - SDL2 https://www.libsdl.org/ 

# Using Digisew-Draw

The main control flow of the program consists of a main drawing window where [voronoi cells](https://en.wikipedia.org/wiki/Voronoi_diagram#:~:text=In%20mathematics%2C%20a%20Voronoi%20diagram,%2C%20sites%2C%20or%20generators) are placed and assigned custom colors to create a voronoi diagram in real-time.
Neighboring cells will have their colors blended together at pixels that do not lie in the center of a cell. Separate layers or "regions" of cells can be specified that are completely independent of one another as well. The end result will be a user-constructed normal map and density map that will be parsed by the Digisew algorithm to output a usable stitch pattern that can be saved and viewed by the program. Each pattern will also be saved in .dst format and can be viewed with any program that can read them. [Embroider modder](https://embroidermodder.org/) or [Embird](https://www.embird.net/download.htm) are potential free methods of reading them.

  **Running the program**
  
  Before running the program, there are some inital parameters that should be set based what you desire out of the instance.
  In the res/params directory, there is a default parameters file called "start_params.txt" that shows the format and inputs for a parameter file.
  To customize parameters to your liking, do either of the following:
  1. Run the program with no console arguments, and set desired values in the default "start_params.txt" file
  2. Run the program with the first console argument specifying the file name of your own parameter .txt file that follows the same format and is in the same directory.
    
Here is a brief explanation of the available parameters:
- width: The width of the main drawing window.
- height: The height of the main drawing window.
- vectorFieldDensityFac: a multiplier/factor for the density of vectors that visualize the stitch directions. A higher value will generate more vectors in the field.
- defaultNormalMap: if provided, any "static" layers specified in the zone map will be **uneditable** and will only display pixels from this image in the normal view
- defaultDensityMap: same as above, but applies to the density map view only.
- zoneMapName: This will specify what "regions" will exist. A region in an area of the drawing window that is completely separate from all other areas, and its points/colors will not affect any other layers or regions in any way. Leaving this blank will place one region across the entire canvas!
	
**Please note** that all pixels that are white (RGB of 255, 255, 255) or have zero opacity (alpha of 0) will be assigned as the "static region", which will be uneditable and will only display pixels from the corresponding default maps provided in the other parameters! Also, regions are dictated based on how many unique pixel values were detected in the zone image. Therefore, images provided should NOT have filtering or heavy compression to work properly. Creating them with a pencil tool in any image editting program will work nicely for this.
    
    
**Controls**
- *Right click*: This will create a selection box that will select any points inside of them. When a point is selected, it will become white and can be interacted with in many different means.
- *Left click*: This will create new points in whatever region currently mouse over if no other points are currently selected. After creating a new point (or when multiple points ARE selected), holding the left mouse and moving it around the click location will set the color of all voronoi cells. The color is based on the direction the mouse is relative to where it was first clicked. You will see the color updated in real time. If in density mode, density of the cells will be set based purely on distance from the initial click point, where further away equates to a darker color & higher density.
- *Middle mouse*: While held, all selected points will be displaced based on mouse movement. Once released, the entire map will rebuild with the new cell posisions
with all other values remaining constant.
- *DELETE key*: Deletes all points currently selected and rebuild the map.
- *S key*: Prompts the user to save whatever is rendered in the currently focused window. Essentially saves a "screenshot" and places it in the output/images directory.
- *D key*: Switches to "density mode", where only the density map is displayed and affected by interaction. Deleting and moving points affects the normal map as well, though, as cells and regions are shared in both modes!
- *F key*: Flips the "polarity" of the color currently being drawn with the mouse. This allows access to the other half of the normal map color space that is otherwise unavailable without polarity flips.
- *H key*: Creates a stitch using the Digisew algorithm with the currently created normal map and density map in the main drawing window. Prompts the user for the name of .dst file to be saved from this action, and then creates a new window with a visualization of the results when complete. Results are saved to output/dst
- *Q key*: Displays voronoi cell borders and intersection points. This is for debugging purposes and is not useful in normal usage of the program.

# Documentation/Program Architecture

Any classes not mentioned below were a part of the legacy Digisew code which I did not author and therefore cannot speak for the exact purpose/implementation of.

* SketchProgram.cpp: The overall program is structured such that an instance of the SketchProgram class is all that is required to start the program. One must be constructed, initialized,
and finally sent into its main loop. The main loop uses a simple SDL2 game loop structure, with an update, render, and event check occuring each frame. Each layer is indvidually evaluated and all pixels determined to overlap that layer are displayed in the final texture.
* SketchLine.cpp: Represents a line being drawn by the user to alter the state of the normal map/density map.
* Layer.cpp: This contains the raw normal map/density map information and assists in voronoi cell generation.
* VoronoiPoint.cpp: Stores its normal color/density values, position, as well as all neighboring cell references. Also knows references to locations where voronoi cell areas "intersect".
* IntersectionNode.cpp: Stores the average color value between all voronoi cells that this point is perfectly equidistant from.
* StitchResult.cpp: Stores the normal map, density map, and resultant stitch map for any given usage of the Digisew algorithm and displays the result in its own window. This is where the digisew algorithm and linkage with the legacy codebase will be found.
* DynamicColor.cpp: Assists in blending pixel values based on its barycentric coordinates, where the vertices are the nearest voronoi point and the two intersection nodes the pixel lies inside of. Think of each intersection node formimg the edge of a triangle, where the third vertex that completes the triangle is the voronoi cell center point. Pixels that lie inside of a given triangle will use the corresponding information to compute the blended color.
* PixelRGB.cpp: Struct that represents a pixel with just RGB channels. It's structured in such a way that instances can be created in a 2D array that is completely contiguous in memory with fast lookup times (no member functions, only static methods and RGB member variables)
* VectorField.cpp: Displays a field of non-directional vectors that rotate based on the encoded normal map direction represented by the color of a given pixel.
* FieldLine.cpp: Attaches itself to a specific pixel on the final texture and rotates itself based on that pixel's color
* Vector2D.cpp: A custom Vector2 class that provides typical vector math utility in a slightly more intuitive fashsion than more advanced implementations.
* Helpers.cpp: Various helper methods used across the program. Most notably, helps translate vectors to normal map colors and vice versa.


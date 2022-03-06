#ifndef SKETCHLINE_H
#define SKETCHLINE_H

#include <utility>
#include <vector>

#include <SDL2/SDL.h>

#include "Vector2D.h"
#include "DynamicColor.h"
#include "Helpers.h"

class SketchLine
{
public:

	SketchLine(const Vector2D& start, const Vector2D& end);
	~SketchLine();
	
	void RenderLine(SDL_Renderer* rend);	
	void SetStartPoint(const Vector2D& start);
	void SetEndPoint(const Vector2D& end);
	//void ColorEncompassedPixels(DynamicColor**& screenColorsArray);

	float Get_Magnitude();			// Length of line.
	Vector2D Get_Origin();			// Origin point, probably where mouse was initially clicked.
	Vector2D Get_Midpoint();		// Midpoint of line.
	Vector2D Get_Direction();		// Direction of line relative to start
	Vector2D Get_Normal();			// Clockwise normal to line.
	SDL_Color Get_RenderColor();	// Current render color based on direction.

private:

	// Represents line direction and position in screen space.
	Vector2D startPos;
	Vector2D endPos;
	Vector2D direction;

	// CW  = rotate 90 deg. clockwise for this direction.
	Vector2D normalCW;

	// Render color that will be based on direction normal.
	SDL_Color rendColor;

	// Positions of pixels surrounding the line. Uses the same radius
	// as the line itself right now (half its magnitude)
	//std::vector<DynamicColor*> encompassedPixels;
};

#endif
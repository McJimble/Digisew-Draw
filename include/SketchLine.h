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

	static const int maxBlueDist;
	static const int maxBlueDistSqr;
	static const int defaultColorDist;
	static const int defaultColorDistSqr;

	SketchLine(const Vector2D& start, const Vector2D& end);
	~SketchLine();
	
	void RenderLine(SDL_Renderer* rend, bool drawCircle = false);	
	void UpdateColor();
	void SetStartPoint(const Vector2D& start);
	void SetEndPoint(const Vector2D& end);
	void SetColorMode(bool enableBlue);
	void SetPolarity(bool pol);
	void FlipPolarity();
	Uint8 GetPixelValueFromDistance(int min, int max);

	bool Get_Polarity();			// True = postive X half of normal map color wheel, False = negative X
	float Get_Magnitude();			// Length of line.
	Vector2D Get_Origin();			// Origin point, probably where mouse was initially clicked.
	Vector2D Get_Midpoint();		// Midpoint of line.
	Vector2D Get_Direction();		// Direction of line relative to start
	Vector2D Get_Normal();			// Clockwise normal to line.
	SDL_Color Get_RenderColor();	// Current render color based on direction.

private:

	

	bool blueSetMode = false;
	bool positivePolarity = true;
	Uint8 restoreR, restoreG;

	// Represents line direction and position in screen space.
	Vector2D startPos;
	Vector2D endPos;
	Vector2D direction;

	// CW  = rotate 90 deg. clockwise for this direction.
	Vector2D normalCW;

	// Render color that will be based on direction normal.
	SDL_Color rendColor;

	SDL_Rect startPosRect;
	SDL_Rect maxRadiusRect;
	// Positions of pixels surrounding the line. Uses the same radius
	// as the line itself right now (half its magnitude)
	//std::vector<DynamicColor*> encompassedPixels;
};

#endif
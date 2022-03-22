#include "SketchLine.h"
#include <iostream>

SketchLine::SketchLine(const Vector2D& start, const Vector2D& end)
{
	startPos	= start;
	endPos		= end;
	direction	= (endPos - startPos).Get_Normalized();

	// Making default normal color for now; temporary?
	Helpers::NormalMapDefaultColor(&rendColor);
}

SketchLine::~SketchLine()
{

}

void SketchLine::RenderLine(SDL_Renderer* rend)
{
	Helpers::Vector2DtoNormalColorHalf(direction, &rendColor);

	SDL_SetRenderDrawColor(rend, rendColor.r, rendColor.g, rendColor.b, rendColor.a);
	SDL_RenderDrawLine(rend, startPos[0], startPos[1], endPos[0], endPos[1]);
}

void SketchLine::SetStartPoint(const Vector2D& start)
{
	startPos = start;
	direction = (endPos - startPos).Get_Normalized();
}

void SketchLine::SetEndPoint(const Vector2D& end)
{
	endPos = end;
	direction = (endPos - startPos).Get_Normalized();
}

//void SketchLine::ColorEncompassedPixels(DynamicColor**& screenColorArray)
//{
//    
//}

float SketchLine::Get_Magnitude()
{
	return (endPos - startPos).Magnitude();
}

Vector2D SketchLine::Get_Origin()
{
	return startPos;
}

Vector2D SketchLine::Get_Midpoint()
{
	return (endPos + startPos) / 2;
}

Vector2D SketchLine::Get_Normal()
{
	return direction.PerpendicularCW();
}

Vector2D SketchLine::Get_Direction()
{
	return direction;
}

SDL_Color SketchLine::Get_RenderColor()
{
	return rendColor;
}
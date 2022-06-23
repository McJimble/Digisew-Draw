#include "SketchLine.h"
#include "Helpers.h"
#include <iostream>

// Chache sqr. distances for faster calculations. Actual dist. to check is 200 pixels.
const int SketchLine::maxBlueDist = 100;
const int SketchLine::maxBlueDistSqr = SketchLine::maxBlueDist * SketchLine::maxBlueDist;
const int SketchLine::defaultColorDist = 10;
const int SketchLine::defaultColorDistSqr = SketchLine::defaultColorDist * SketchLine::defaultColorDist;

SketchLine::SketchLine(const Vector2D& start, const Vector2D& end)
{
	startPos	= start;
	endPos		= end;
	direction	= (endPos - startPos).Get_Normalized();
	
	startPosRect.w = startPosRect.h = defaultColorDist;
	startPosRect.x = startPos[0] - (defaultColorDist * 0.5);
	startPosRect.y = startPos[1] - (defaultColorDist * 0.5);

	// Rect in case we want to draw rects using these values...
	maxRadiusRect.w = maxRadiusRect.h = maxBlueDist;
	maxRadiusRect.x = startPos[0];// -(maxBlueDist * 0.5); <- if using for rect, change this.
	maxRadiusRect.y = startPos[1];// -(maxBlueDist * 0.5);

	// Making default normal color for now; temporary?
	Helpers::NormalMapDefaultColor(&rendColor);
}

SketchLine::~SketchLine()
{

}

void SketchLine::RenderLine(SDL_Renderer* rend, bool drawCircle)
{
	SDL_SetRenderDrawColor(rend, 255, 255, 255, 255);
	SDL_RenderFillRect(rend, &startPosRect);

	if (blueSetMode || drawCircle)
		Helpers::SDL_DrawCircle(rend, maxRadiusRect.x, maxRadiusRect.y, maxRadiusRect.w);

	// In blue set mode, render with just varying blue channel so the line is easier to
	// see. The actual render color used in setting voronoi colors is unchanged though.
	SDL_SetRenderDrawColor(rend, (blueSetMode) ? 128 : rendColor.r, (blueSetMode) ? 128 : rendColor.g, rendColor.b, rendColor.a);
	SDL_RenderDrawLine(rend, startPos[0], startPos[1], endPos[0], endPos[1]);
}

void SketchLine::UpdateColor()
{
	if (blueSetMode)
	{
		rendColor.r = restoreR;
		rendColor.g = restoreG;
		rendColor.b = GetPixelValueFromDistance(128, 255);
	}
	else if ((endPos - startPos).SqrMagnitude() < defaultColorDistSqr)
	{
		Helpers::NormalMapDefaultColor(&rendColor);
	}
	else
	{
		Helpers::Vector2DtoNormalColorHalf(direction, &rendColor, positivePolarity);
		restoreR = rendColor.r;
		restoreG = rendColor.g;
	}

	//std::cout << (int)rendColor.r << ", " << (int)rendColor.g  << ", " << (int)rendColor.b << "\n";
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

void SketchLine::SetColorMode(bool enableBlue)
{
	blueSetMode = enableBlue;
}

void SketchLine::SetPolarity(bool pol)
{
	positivePolarity = pol;
}

void SketchLine::FlipPolarity()
{
	positivePolarity = !positivePolarity;
}

Uint8 SketchLine::GetPixelValueFromDistance(int minVal = 0, int maxVal = 255)
{
	float len = (endPos - startPos).SqrMagnitude();
	float t = Helpers::InverseLerp(0, maxBlueDistSqr, len);
	t = Helpers::Clamp(t, 0.0f, 1.0f);

	return Helpers::Lerp(minVal, maxVal, t);
}

bool SketchLine::Get_Polarity()
{
	return positivePolarity;
}

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
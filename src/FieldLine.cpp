#include "FieldLine.h"
#include "Helpers.h"

#include <iostream>
FieldLine::FieldLine(DynamicColor* colorToRead, const Vector2D& pos, float length)
{
	this->colorToRead	= colorToRead;
	this->position		= pos;
	this->length		= length;
	this->initialLength	= length;
}

FieldLine::~FieldLine()
{

}

void FieldLine::UpdateLine()
{
	// Don't update if we have no color to update with.
	if (!colorToRead) return;

	const PixelRGB* readThisFrame = colorToRead->Get_AffectedPixel();
	Vector2D voronoiPos = colorToRead->Get_MinPoint()->Get_Position();

	//direction = Helpers::HalfNormalColorToDirection(readThisFrame->r, readThisFrame->g, colorToRead->Get_MinPoint()->Get_PolaritySwapped());
	direction = Vector2D(Helpers::InverseLerp(128.0f, 255.0f, readThisFrame->r), -Helpers::InverseLerp(128.0f, 255.0f, readThisFrame->g)).Get_Normalized();
	length = (Helpers::InverseLerp(255, 128, readThisFrame->b)) * initialLength;

	if (readThisFrame->r == 128 && readThisFrame->g == 128)
	{
		direction = Vector2D(0.0f, 0.0f);
	}

	startPoint = position - (direction * length * 0.5f);
	endPoint = startPoint + (direction * length);
}

void FieldLine::RenderLine(SDL_Renderer* rend)
{

	SDL_RenderDrawLineF(rend, startPoint[0], startPoint[1], endPoint[0], endPoint[1]);
}

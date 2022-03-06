#include "FieldLine.h"
#include "Helpers.h"

FieldLine::FieldLine(const std::shared_ptr<DynamicColor>& colorToRead, const Vector2D& pos, float length)
{
	this->colorToRead	= colorToRead;
	this->position		= pos;
	this->length		= length;
}

FieldLine::~FieldLine()
{

}

void FieldLine::UpdateLine()
{
	// Don't update if we have no color to update with.
	if (!colorToRead) return;

	direction = Helpers::HalfNormalColorToDirection(colorToRead->Get_AffectedPixel()->r, colorToRead->Get_AffectedPixel()->g);

	startPoint = position - (direction * length * 0.5f);
	endPoint = startPoint + (direction * length);
}

void FieldLine::RenderLine(SDL_Renderer* rend)
{
	SDL_RenderDrawLineF(rend, startPoint[0], startPoint[1], endPoint[0], endPoint[1]);
}

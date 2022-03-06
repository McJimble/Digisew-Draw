#pragma once
#include <vector>
#include <memory>
#include <SDL2/SDL.h>

#include "DynamicColor.h"
#include "Vector2D.h"
#include "PixelRGB.h"

/*
 *	A single field line contained by a vector field 
 */
class FieldLine
{
public:

	FieldLine(const std::shared_ptr<DynamicColor>& colorToRead, const Vector2D& pos, float length = 5.0f);
	~FieldLine();

	// Recalculates render positions and direction when told.
	void UpdateLine();

	// Renders line based on last calculated start, end, direction
	void RenderLine(SDL_Renderer* rend);

private:

	std::shared_ptr<DynamicColor> colorToRead;

	float length;
	Vector2D position;
	Vector2D startPoint;
	Vector2D endPoint;
	Vector2D direction;
};
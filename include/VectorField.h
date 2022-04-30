#pragma once

#include "FieldLine.h"

/*
 *	Holds and displays vectors whose directions change based on the color of pixels. 
 */
class VectorField
{
public:

	/*
	 *	sizeX/sizeY:			Number of vectors to display on the x and y axes.
	 *	padding:				Number of pixels from resolution margin to space edge vectors from.
	 *  renderColor:			Color to display each vector in the field as.
	 */
	VectorField(int sizeX, int sizeY, int padding, SDL_Color renderColor);
	~VectorField();

	/*
	 *	Initializes all vectors in the field with a dynamic color that they will read the values of when updated.
	 *	A vector will pick the closest screen space pixel to its position.
	 */
	void InitializeVectors(const std::vector<std::vector<DynamicColor*>>& pixmap, float lineLengths = 5.0f);

	/*
	 *	Updates all vectors in the vector field at once based on r & g values found
	 *  at pixels they were initialized with.
	 */
	void UpdateAll();

	/*
	 *	Renders the entire vector field
	 */
	void Render(SDL_Renderer* rend);

private:

	std::vector<std::vector<FieldLine*>> fieldLines;

	int sizeX, sizeY;
	int padding;
	SDL_Color rendColor;
};

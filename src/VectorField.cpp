#include "VectorField.h"
#include <iostream>

VectorField::VectorField(int sizeX, int sizeY, int padding, SDL_Color renderColor)
{
	this->sizeX					= sizeX;
	this->sizeY					= sizeY;
	this->padding				= padding;
	this->rendColor				= renderColor;
}

VectorField::~VectorField()
{

}

void VectorField::InitializeVectors(PixelRGB**& pixmap, int pixelsX, int pixelsY, float lineLength)
{
    int maxX = pixelsX - padding - padding;
    int maxY = pixelsY - padding - padding;

    fieldLines.resize(sizeX);
    for (int x = 0; x < sizeX; x++)
    {
        std::vector<FieldLine*> temp;
        temp.reserve(sizeY);
        for (int y = 0; y < sizeY; y++)
        {
            float interpX = (float)x / (sizeX - 1);
            float interpY = (float)y / (sizeY - 1);
            Vector2D pos = Vector2D((interpX * maxX) + padding, (interpY * maxY) + padding);

            int pixX = (int)pos[0];
            int pixY = (int)pos[1];
            
            FieldLine* newLine = new FieldLine(&pixmap[pixY][pixX], pos, lineLength);
            temp.push_back(newLine);
        }
        fieldLines[x].insert(fieldLines[x].end(), temp.begin(), temp.end());
    }
}

void VectorField::UpdateAll()
{
    for (auto& lineX : fieldLines)
    {
        for (auto& lineY : lineX)
        {
            lineY->UpdateLine();
        }
    }
}

void VectorField::Render(SDL_Renderer* rend)
{
    // Render all lines as this color.
    SDL_SetRenderDrawColor(rend, rendColor.r, rendColor.g, rendColor.b, rendColor.a);

    for (auto& lineX : fieldLines)
    {
        for (auto& lineY : lineX)
        {
            lineY->RenderLine(rend);
        }
    }
}
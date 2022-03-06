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

void VectorField::InitializeVectors(const std::vector<std::vector<std::shared_ptr<DynamicColor>>>& pixmap, float lineLength)
{
    int maxX = pixmap.size() - padding;
    int maxY = pixmap[0].size() - padding;
    int spacingX = (maxX - padding) / sizeX;
    int spacingY = (maxY - padding) / sizeY;

    fieldLines.resize(sizeX);
    for (int x = 0; x < sizeX; x++)
    {
        std::vector<std::shared_ptr<FieldLine>> temp;
        temp.reserve(sizeY);
        for (int y = 0; y < sizeY; y++)
        {
            Vector2D pos = Vector2D((x * spacingX) + padding, (y * spacingY) + padding);
            
            int pixX = (int)pos[0];
            int pixY = (int)pos[1];
            
            FieldLine* newLine = new FieldLine(pixmap[pixX][pixY], pos, lineLength);
            temp.push_back(std::shared_ptr<FieldLine>(newLine));
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
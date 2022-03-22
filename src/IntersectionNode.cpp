#include "IntersectionNode.h"
#include "Vector2D.h"
#include "VoronoiPoint.h"
#include "Helpers.h"

#include <memory>
#include <iostream>

int IntersectionNode::maxPoints = 3;
int IntersectionNode::nextID = 0;

IntersectionNode::IntersectionNode(const Vector2D& setPosition, const std::vector<VoronoiPoint*>& intersectingPoints)
{
	this->position = setPosition;
	this->identifier = nextID++;
	this->intersectingPoints = intersectingPoints;
	UpdateColor();
}

void IntersectionNode::UpdateColor()
{
	int size = intersectingPoints.size();

	if (size <= 0) return;

	// Temp hardcoded color to test positions/barycentric coordinates
	//Helpers::NormalMapDefaultColor(&averageColor);
	//return;

	int sumR, sumG, sumB;
	sumR = sumG = sumB = 0;
	for (int i = 0; i < size; i++)
	{
		// Summing color channels to enable finding average later.
		const PixelRGB& currColor = intersectingPoints[i]->Get_NormalEncoding();
		sumR += currColor.r;
		sumG += currColor.g;
		sumB += currColor.b;
	}

	if (sumR == 0 && sumG == 0 && sumB == 0)
		std::cout << "Error when updating color; black is invalid but was calculated anyways\n";

	this->averageColor.r = (Uint8)(sumR / size);
	this->averageColor.g = (Uint8)(sumG / size);
	this->averageColor.b = (Uint8)(sumB / size);
}

void IntersectionNode::RenderNode(SDL_Renderer* rend)
{
	// Rect center drawn at point with arbitrary size; centered around pos.
	SDL_FRect drawRect;
	drawRect.x = position[0] - 2.5;
	drawRect.y = position[1] - 2.5;
	drawRect.w = 5;
	drawRect.h = 5;

	SDL_SetRenderDrawColor(rend, 255, 255, 255, 255);
	SDL_RenderDrawRectF(rend, &drawRect);
}

int IntersectionNode::Get_ID() const
{
	return identifier;
}

const Vector2D& IntersectionNode::Get_Position() const 
{
	return position;
}

const PixelRGB& IntersectionNode::Get_AverageColor() const
{
	return averageColor;
}

bool IntersectionNode::EnvelopesSamePoints(const IntersectionNode& other) const
{
	if (other.intersectingPoints.size() != this->intersectingPoints.size()) return false;

	for (auto& ptOth : other.intersectingPoints)
	{
		bool match = false;
		for (auto& pt : intersectingPoints)
		{
			if (ptOth->Get_ID() == pt->Get_ID())
			{
				match = true;
				break;
			}
		}
		if (!match) 
			return false;
	}

	return true;
}
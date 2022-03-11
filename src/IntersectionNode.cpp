#include "IntersectionNode.h"
#include "Vector2D.h"
#include "VoronoiPoint.h"

#include <memory>

int IntersectionNode::maxPoints = 3;
int IntersectionNode::nextID = 0;

IntersectionNode::IntersectionNode(const Vector2D& setPosition, std::vector<VoronoiPoint*> intersectingPoints)
{
	int size = intersectingPoints.size(); // Caching; using it a lot.

	// Each point should be same distance, but doing this for safetly against miniscule differences
	float maxDistance = DBL_MIN;	
	int sumR, sumG, sumB;
	sumR = sumG = sumB = 0;
	for (int i = 0; i < size; i++)
	{
		float dist = (intersectingPoints[i]->Get_Position() - setPosition).SqrMagnitude();
		this->intersectingPoints.emplace_back(dist, std::shared_ptr<VoronoiPoint>(intersectingPoints[i]));
	
		if (dist > maxDistance) maxDistance = dist;

		// Summing color channels to enable finding average later.
		const PixelRGB& currColor = intersectingPoints[i]->Get_NormalEncoding();
		sumR += currColor.r;
		sumG += currColor.g;
		sumB += currColor.b;
	}
	this->averageColor.r = (Uint8)(sumR / size);
	this->averageColor.g = (Uint8)(sumG / size);
	this->averageColor.b = (Uint8)(sumB / size);

	this->position = setPosition;
	this->pointsDistance = maxDistance;
	this->identifier = nextID++;
}

void IntersectionNode::UpdateColor()
{
	int size = intersectingPoints.size();
	int sumR, sumG, sumB;
	sumR = sumG = sumB = 0;
	for (int i = 0; i < size; i++)
	{
		// Summing color channels to enable finding average later.
		const PixelRGB& currColor = intersectingPoints[i].second->Get_NormalEncoding();
		sumR += currColor.r;
		sumG += currColor.g;
		sumB += currColor.b;
	}

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

bool IntersectionNode::CheckForShouldDissolve(VoronoiPoint* newPoint)
{
	if ((position - newPoint->Get_Position()).SqrMagnitude() > pointsDistance)
	{
		return false;
	}

	return true;
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
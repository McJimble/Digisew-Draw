#include "IntersectionNode.h"

int IntersectionNode::maxPoints = 3;

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

	this->position = setPosition;
	this->averageColor = SDL_Color{ (Uint8)(sumR / size), (Uint8)(sumG / size), (Uint8)(sumB / size) , 255};
	this->pointsDistance = maxDistance;
}

IntersectionNode::~IntersectionNode()
{

}

bool IntersectionNode::CheckForShouldDissolve(VoronoiPoint* newPoint)
{
	if ((position - newPoint->Get_Position()).SqrMagnitude() > pointsDistance)
	{
		return false;
	}

	return true;
}

const Vector2D& IntersectionNode::Get_Position()
{
	return position;
}

const SDL_Color& IntersectionNode::Get_AverageColor()
{
	return averageColor;
}
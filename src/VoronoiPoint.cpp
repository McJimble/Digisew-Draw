#include "VoronoiPoint.h"
#include <algorithm>

int VoronoiPoint::pixelRadius = 2;

VoronoiPoint::VoronoiPoint(const Vector2D& position, const PixelRGB& normalEncoding, int zone)
{
	this->normalEncoding	= normalEncoding;
	this->position			= position;
	this->zoneIndex			= zone;
}

VoronoiPoint::VoronoiPoint(const Vector2D& position, const SDL_Color& normalEncoding, int zone)
{
	this->normalEncoding.r = normalEncoding.r;
	this->normalEncoding.g = normalEncoding.g;
	this->normalEncoding.b = normalEncoding.b;

	this->position	= position;
	this->zoneIndex = zone;
}

void VoronoiPoint::RenderPoint(SDL_Renderer* rend)
{
	for (int x = -pixelRadius; x < pixelRadius; x++)
		for (int y = -pixelRadius; y < pixelRadius; y++)
		{
			SDL_RenderDrawPoint(rend, position[0] + x, position[1] + y);
		}
}

void VoronoiPoint::AddNode(IntersectionNode* node)
{
	if (neighboringNodes.empty())
	{
		neighboringNodes.push_back(std::shared_ptr<IntersectionNode>(node));
		return;
	}

	// Add nodes into vector in order of relative angle; this allows us to iterate
	// through the triangles this creates by just iterating from start to end.
	float newAngle = (node->Get_Position() - position).Angle();
	for (int i = 0; i < neighboringNodes.size(); i++)
	{
		float neighborAngle = (neighboringNodes[i]->Get_Position() - position).Angle();
		if (neighborAngle < newAngle)
		{
			neighboringNodes.insert(neighboringNodes.begin() + i, std::shared_ptr<IntersectionNode>(node));
			break;
		}

		if (i == neighboringNodes.size() - 1)
		{
			neighboringNodes.push_back(std::shared_ptr<IntersectionNode>(node));
			break;
		}
	}
}

bool VoronoiPoint::RemoveNode(IntersectionNode* node)
{
	std::shared_ptr<IntersectionNode> temp(node);
	auto position = std::find(neighboringNodes.begin(), neighboringNodes.end(), temp);
	if (position != neighboringNodes.end())
	{
		neighboringNodes.erase(position);
		return true;
	}
	return false;
}

const std::vector<std::shared_ptr<IntersectionNode>>& VoronoiPoint::Get_NeighboringNodes() const
{
	return neighboringNodes;
}

const int VoronoiPoint::Get_VoronoiZone() const
{
	return zoneIndex;
}

const Vector2D& VoronoiPoint::Get_Position() const
{
	return position;
}

const PixelRGB& VoronoiPoint::Get_NormalEncoding() const
{
	return normalEncoding;
}

void VoronoiPoint::Set_VoronoiZone(int zone)
{
	zoneIndex = zone;
}

void VoronoiPoint::Set_NormalEncoding(const SDL_Color& normalEncoding)
{
	this->normalEncoding.r = normalEncoding.r;
	this->normalEncoding.g = normalEncoding.g;
	this->normalEncoding.b = normalEncoding.b;

	// Signal to nodes that this color has changed, so it should update.
	for (auto& node : neighboringNodes)
	{
		node->UpdateColor();
	}
}

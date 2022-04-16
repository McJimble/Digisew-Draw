#include "VoronoiPoint.h"
#include <algorithm>

int VoronoiPoint::pixelRadius = 2;
int VoronoiPoint::nextID = 0;

VoronoiPoint::VoronoiPoint(const Vector2D& position, const PixelRGB& normalEncoding, int zone)
{
	this->normalEncoding	= normalEncoding;
	this->position			= position;
	this->zoneIndex			= zone;
	this->id = nextID++;
	this->renderColor = { 0, 0, 0, 255 };
}

VoronoiPoint::VoronoiPoint(const Vector2D& position, const SDL_Color& normalEncoding, int zone)
{
	this->normalEncoding.r = normalEncoding.r;
	this->normalEncoding.g = normalEncoding.g;
	this->normalEncoding.b = normalEncoding.b;

	this->position	= position;
	this->zoneIndex = zone;
	this->id = nextID++;
	this->renderColor = { 0, 0, 0, 255 };
}

void VoronoiPoint::RenderPoint(SDL_Renderer* rend)
{
	SDL_SetRenderDrawColor(rend, renderColor.r, renderColor.g, renderColor.b, renderColor.a);
	for (int x = -pixelRadius; x < pixelRadius; x++)
		for (int y = -pixelRadius; y < pixelRadius; y++)
		{
			SDL_RenderDrawPoint(rend, position[0] + x, position[1] + y);
		}
}

void VoronoiPoint::RenderFormedTriangles(SDL_Renderer* rend)
{
	SDL_Color oldCol;
	SDL_GetRenderDrawColor(rend, &oldCol.r, &oldCol.g, &oldCol.b, &oldCol.a);
	SDL_SetRenderDrawColor(rend, 0, 0, 0, 255);

	for (int i = 0; i < neighboringNodes.size(); i++)
	{
		Vector2D cur = neighboringNodes[i]->Get_Position();
		Vector2D nxt = neighboringNodes[(i + 1) % neighboringNodes.size()]->Get_Position();
		SDL_RenderDrawLine(rend, cur[0], cur[1], nxt[0], nxt[1]);

		//SDL_RenderDrawLine(rend, position[0], position[1], cur[0], cur[1]);
		//SDL_RenderDrawLine(rend, position[0], position[1], nxt[0], nxt[1]);
	}

	SDL_SetRenderDrawColor(rend, oldCol.r, oldCol.g, oldCol.b, oldCol.a);
}

void VoronoiPoint::AddNode(IntersectionNode* node)
{
	if (neighboringNodes.empty())
	{
		neighboringNodes.push_back(node);
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
			neighboringNodes.insert(neighboringNodes.begin() + i, node);
			break;
		}

		if (i == neighboringNodes.size() - 1)
		{
			neighboringNodes.push_back(node);
			break;
		}
	}
}

bool VoronoiPoint::RemoveNode(IntersectionNode* node)
{
	auto position = std::find(neighboringNodes.begin(), neighboringNodes.end(), node);
	if (position != neighboringNodes.end())
	{
		neighboringNodes.erase(position);
		return true;
	}
	return false;
}

const std::vector<IntersectionNode*>& VoronoiPoint::Get_NeighboringNodes() const
{
	return neighboringNodes;
}

const int VoronoiPoint::Get_ID() const
{
	return id;
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

void VoronoiPoint::Set_RenderColor(const SDL_Color& renderColor)
{
	this->renderColor = renderColor;
}

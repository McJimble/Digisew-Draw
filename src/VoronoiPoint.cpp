#include "VoronoiPoint.h"

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

VoronoiPoint::~VoronoiPoint()
{

}

void VoronoiPoint::RenderPoint(SDL_Renderer* rend)
{
	for (int x = -pixelRadius; x < pixelRadius; x++)
		for (int y = -pixelRadius; y < pixelRadius; y++)
		{
			SDL_RenderDrawPoint(rend, position[0] + x, position[1] + y);
		}
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
}

#include "DynamicColor.h"
#include <iostream>

float DynamicColor::edgeThreshold = 0.99f;

DynamicColor::DynamicColor(PixelRGB* affectedPixel, const Vector2D& position)
{
    this->affectedPixel     = affectedPixel;
    this->pixPosition       = position;
    this->voronoiDensity    = 1.0f;
    this->voronoiZone       = 0;
}

DynamicColor::~DynamicColor()
{
    //potentialColors.clear();
    affectedPixel = nullptr;
}

void DynamicColor::UpdatePixel()
{
    // Base color is that of closest point if we have one stored. If not, it's the default color.
    PixelRGB defaultColor;
    defaultColor = Helpers::NormalMapDefaultColor();

    PixelRGB::CopyData((minPt) ? &minPt->Get_NormalEncoding() : &defaultColor, affectedPixel);

    // The closer the distance is to being equidistant to two points, 
    // the closer the density value is to one.
    voronoiDensity = (!secondMinPt) ? 0.0f : ( minPtDistance / secondMinPtDistance );
    voronoiDensity = std::min(1.0f, voronoiDensity);

    // Make pixel black if equidistant to second point
    //PixelRGB black = PixelRGB::MakePixel(0, 0, 0);
    //PixelRGB::CopyData((voronoiDensity > edgeThreshold) ? &black : affectedPixel, affectedPixel);

    //DensityToLuminosity();
    //DensityToColor();
}

bool DynamicColor::TryAddMinPoint(const std::shared_ptr<VoronoiPoint>& newPoint)
{
    if (newPoint->Get_VoronoiZone() != this->voronoiZone) return false;

    // Check cached distances to currently stored points.
    // if less than min, store min & position, make old min the new 2nd min
    // else if less than 2ndMin, store as 2ndMin & position, ret true.
    float sqrDistNew = (newPoint->Get_Position() - pixPosition).SqrMagnitude();
    if (sqrDistNew < minPtDistance)
    {
        secondMinPt = (minPt) ? minPt : secondMinPt;
        secondMinPtDistance = minPtDistance;

        minPt = newPoint;
        minPtDistance = sqrDistNew;
        return true;
    }
    else if (sqrDistNew < secondMinPtDistance)
    {
        secondMinPt = newPoint;
        secondMinPtDistance = sqrDistNew;
        return true;
    }

    return false;
}

void DynamicColor::UpdatePixelInterp(const PixelRGB* newColor, float t)
{
    PixelRGB interpColor = Helpers::LerpColorRGB(*newColor, *affectedPixel, t);
    PixelRGB::CopyData(&interpColor, affectedPixel);
}

void DynamicColor::DensityToLuminosity()
{
    unsigned char val = (unsigned char)(voronoiDensity * 255);
    PixelRGB densityColorVal = PixelRGB::MakePixel(val, val, val);
    PixelRGB::CopyData(&densityColorVal, affectedPixel);
}

void DynamicColor::DensityToColor()
{
    // Old code that just lowered the value on hsv space.
    /*
    float h, s, v;
    Helpers::RGBtoHSV(h, s, v, affectedPixel);

    v = Helpers::Lerp(0.3f, 1.0f, 1 - voronoiDensity);
    Helpers::HSVtoRGB(h, s, v, affectedPixel);
    */

    float remapDensity = Helpers::Lerp(0.0f, 0.5f, voronoiDensity);
    Helpers::LerpColor(minPt->Get_NormalEncoding(),
                       (secondMinPt) ? secondMinPt->Get_NormalEncoding() : minPt->Get_NormalEncoding(),
                       remapDensity,
                       affectedPixel);
                     
}

void DynamicColor::SetVoronoiZone(int newZoneID)
{
    this->voronoiZone = newZoneID;
}

float DynamicColor::Get_VornoiDensity()
{
    return voronoiDensity;
}

const PixelRGB* DynamicColor::Get_AffectedPixel()
{
    return affectedPixel;
}

const Vector2D& DynamicColor::Get_PixelPosition()
{
    return pixPosition;
}

VoronoiPoint* DynamicColor::Get_MinPoint()
{
    return minPt.get();
}
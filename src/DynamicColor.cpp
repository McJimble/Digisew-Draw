#include "DynamicColor.h"
#include "Vector3D.h"
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
    BarycentricToColor();
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

void DynamicColor::BarycentricToColor()
{
    if (!minPt || !triNodeA || !triNodeB) return; // For sanity
    Vector3D additiveColor;
    const PixelRGB& colA = minPt->Get_NormalEncoding();
    const PixelRGB& colB = triNodeA->Get_AverageColor();
    const PixelRGB& colC = triNodeB->Get_AverageColor();

    additiveColor[0] = Helpers::Clamp(baryU * colA.r + baryV * colB.r + baryW * colC.r, 0, 255);
    additiveColor[1] = Helpers::Clamp(baryU * colA.g + baryV * colB.g + baryW * colC.g, 0, 255);
    additiveColor[2] = Helpers::Clamp(baryU * colA.b + baryV * colB.b + baryW * colC.b, 0, 255);

    affectedPixel->r = (Uint8)additiveColor[0];
    affectedPixel->g = (Uint8)additiveColor[1];
    affectedPixel->b = (Uint8)additiveColor[2];
}

void DynamicColor::SetVoronoiZone(int newZoneID)
{
    this->voronoiZone = newZoneID;
}

bool DynamicColor::Set_TriangulationNodes(IntersectionNode* a, IntersectionNode* b, const Vector2D& origin)
{
    Helpers::ComputeBarycentricCoordinates(pixPosition, origin, a->Get_Position(), b->Get_Position(),
        baryU, baryV, baryW);

    triNodeA = a;
    triNodeB = b;
    baryU = Helpers::Clamp(baryU, 0.0f, 1.0f);
    baryV = Helpers::Clamp(baryV, 0.0f, 1.0f);
    baryW = Helpers::Clamp(baryW, 0.0f, 1.0f);
    return true;
}

bool DynamicColor::ContainsNode(IntersectionNode* node)
{
    return (node == triNodeA || node == triNodeB);
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
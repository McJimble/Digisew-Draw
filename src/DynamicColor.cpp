#include "DynamicColor.h"
#include <iostream>

float DynamicColor::edgeThreshold = 0.99f;

DynamicColor::DynamicColor(PixelRGB* affectedPixel, PixelRGB* densityPixel, const Vector2D& position)
{
    this->affectedPixel     = affectedPixel;
    this->densityPixel      = densityPixel;
    this->pixPosition       = position;
    this->voronoiDensity    = 1.0f;
    this->voronoiZone       = 0;

    // Pixels are initialized with normal map default, so fix that for density pixel.
    this->densityPixel->r = 128;
    this->densityPixel->g = 128;
    this->densityPixel->b = 128;
}

DynamicColor::~DynamicColor()
{
}

void DynamicColor::UpdatePixel()
{
    // Base color is that of closest point if we have one stored. If not, it's the default color.
    PixelRGB defaultColor;
    defaultColor = Helpers::NormalMapDefaultColor();

    PixelRGB::Copy((minPt) ? &minPt->Get_NormalEncoding() : &defaultColor, affectedPixel);

    // Make pixel black if equidistant to second point
    //PixelRGB black = PixelRGB::MakePixel(0, 0, 0);
    //PixelRGB::CopyData((voronoiDensity > edgeThreshold) ? &black : affectedPixel, affectedPixel);

    BarycentricToColor();
    BarycentricToDensity();
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
        secondMinPtDistance = minPtDistance;

        minPt = newPoint;
        minPtDistance = sqrDistNew;
        return true;
    }

    return false;
}

void DynamicColor::UpdatePixelInterp(const PixelRGB* newColor, float t)
{
    PixelRGB interpColor = Helpers::LerpColorRGB(*newColor, *affectedPixel, t);
    PixelRGB::Copy(&interpColor, affectedPixel);
}

// Deprecated
void DynamicColor::DensityToLuminosity()
{
    /*
    unsigned char val = (unsigned char)(voronoiDensity * 255);
    PixelRGB densityColorVal = PixelRGB::MakePixel(val, val, val);
    PixelRGB::CopyData(&densityColorVal, affectedPixel);
    */
}

// Deprecated function
void DynamicColor::DensityToColor()
{
    // Old code that just lowered the value on hsv space.
    /*
    float h, s, v;
    Helpers::RGBtoHSV(h, s, v, affectedPixel);

    v = Helpers::Lerp(0.3f, 1.0f, 1 - voronoiDensity);
    Helpers::HSVtoRGB(h, s, v, affectedPixel);
    */

    /*
    float remapDensity = Helpers::Lerp(0.0f, 0.5f, voronoiDensity);
    Helpers::LerpColor(minPt->Get_NormalEncoding(),
                       (secondMinPt) ? secondMinPt->Get_NormalEncoding() : minPt->Get_NormalEncoding(),
                       remapDensity,
                       affectedPixel);
                       */
}

void DynamicColor::BarycentricToColor()
{
    if (!minPt || !triNodeA || !triNodeB) return; // For sanity
    double r, g, b;
    const PixelRGB& colA = minPt->Get_NormalEncoding();
    const PixelRGB& colB = triNodeA->Get_AverageColor();
    const PixelRGB& colC = triNodeB->Get_AverageColor();

    r = Helpers::Clamp(baryU * colA.r + baryV * colB.r + baryW * colC.r, 0, 255);
    g = Helpers::Clamp(baryU * colA.g + baryV * colB.g + baryW * colC.g, 0, 255);
    b = Helpers::Clamp(baryU * colA.b + baryV * colB.b + baryW * colC.b, 0, 255);

    affectedPixel->r = r;
    affectedPixel->g = g;
    affectedPixel->b = b;
}

// I know this could just be added in the same function as above, 
// but I split to two just in case it was helpful.
void DynamicColor::BarycentricToDensity()
{
    if (!minPt || !triNodeA || !triNodeB) return; // For sanity
    float densMin = minPt->Get_VoronoiDensity();
    float densA = triNodeA->Get_AverageDensity();
    float densB = triNodeB->Get_AverageDensity();

    float addAverage = Helpers::Clamp(baryU * densMin + baryV * densA + baryW * densB, 0, 255);
    densityPixel->r = (Uint8)addAverage;
    densityPixel->g = densityPixel->r;
    densityPixel->b = densityPixel->r;
}

void DynamicColor::SetVoronoiZone(int newZoneID)
{
    this->voronoiZone = newZoneID;
}

bool DynamicColor::Set_TriangulationNodes(const std::shared_ptr<IntersectionNode>& a, const std::shared_ptr<IntersectionNode>& b, const Vector2D& origin)
{
    Helpers::ComputeBarycentricCoordinates(pixPosition, origin, a->Get_Position(), b->Get_Position(),
        baryU, baryV, baryW);

    triNodeA = std::shared_ptr<IntersectionNode>(a);
    triNodeB = std::shared_ptr<IntersectionNode>(b);
    baryU = Helpers::Clamp(baryU, 0.0f, 1.0f);
    baryV = Helpers::Clamp(baryV, 0.0f, 1.0f);
    baryW = Helpers::Clamp(baryW, 0.0f, 1.0f);
    return true;
}

bool DynamicColor::ContainsNode(IntersectionNode* node)
{
    return (node->Get_ID() == triNodeA->Get_ID() || node->Get_ID() == triNodeB->Get_ID());
}

void DynamicColor::ClearVoronoiData(bool resetColor)
{
    triNodeA.reset();
    triNodeB.reset();
    minPt.reset();
    minPtDistance = FLT_MAX;
    baryU = 0;
    baryV = 0;
    baryW = 0;

    if (resetColor)
        Helpers::NormalMapDefaultColor(affectedPixel);
}

int DynamicColor::Get_VoronoiZone()
{
    return voronoiZone;
}

float DynamicColor::Get_VornoiDensity()
{
    return voronoiDensity;
}

PixelRGB* DynamicColor::Get_AffectedPixel() const
{
    return affectedPixel;
}

const Vector2D& DynamicColor::Get_PixelPosition() const
{
    return pixPosition;
}

VoronoiPoint* DynamicColor::Get_MinPoint() const
{
    return minPt.get();
}

IntersectionNode* DynamicColor::Get_TriNodeA() const
{
    return triNodeA.get();
}

IntersectionNode* DynamicColor::Get_TriNodeB() const
{
    return triNodeB.get();
}

void DynamicColor::Set_AffectedPixels(PixelRGB* normalPix, PixelRGB* densityPix)
{
    affectedPixel = normalPix;
    densityPixel = densityPix;
}
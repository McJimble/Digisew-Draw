#include "Helpers.h"
#include <iostream>


// I know it's better to interpolate within HSV space, but not a priority yet.
// Maybe add it in later so that interpolated/blended colors are more accurate?
PixelRGB Helpers::LerpColorRGB(const PixelRGB& p1, const PixelRGB& p2, float t)
{
    PixelRGB newColor;
    newColor.r = (unsigned char)((p1.r - p2.r) * t + p2.r);
    newColor.g = (unsigned char)((p1.g - p2.g) * t + p2.g);
    newColor.b = (unsigned char)((p1.b - p2.b) * t + p2.b);

    return newColor;
}

PixelRGB Helpers::SDLColorToPixel(const SDL_Color& col)
{
    PixelRGB newColor;
    newColor.r = col.r;
    newColor.g = col.g;
    newColor.b = col.b;

    return newColor;
}

SDL_Color Helpers::PixelToSDLColor(const PixelRGB& pix)
{
    SDL_Color col;
    col.r = pix.r;
    col.g = pix.g;
    col.b = pix.b;
    col.a = 255;

    return col;
}

void Helpers::NormalMapDefaultColor(SDL_Color* ret)
{
    ret->r = 128;
    ret->g = 128;
    ret->b = 255;
    ret->a = 255;
}

void Helpers::NormalMapDefaultColor(PixelRGB* ret)
{
    ret->r = 128;
    ret->g = 128;
    ret->b = 255;
}

PixelRGB Helpers::NormalMapDefaultColor()
{
    PixelRGB ret;
    ret.r = 128;
    ret.g = 128;
    ret.b = 255;

    return ret;
}

// For the record I didn't make this function or the reverse version since this
// conversion is a whole process I didn't want to mess up from doing on my own.
void Helpers::HSVtoRGB(float h, float s, float v, PixelRGB* c)
{
    double hh, p, q, t, ff;
    long i;

    if (s <= 0.0)
    {
        c->r = v;
        c->g = v;
        c->b = v;
        return;
    }

    hh = h;
    if (hh >= 360.0) hh = 0.0;
    hh /= 60.0;
    i = (long)hh;
    ff = hh - i;

    v *= 255;
    p = v * (1.0 - s);
    q = v * (1.0 - (s * ff));
    t = v * (1.0 - (s * (1.0 - ff)));

    switch (i) {
        case 0:
            c->r = v;
            c->g = t;
            c->b = p;
            break;
        case 1:
            c->r = q;
            c->g = v;
            c->b = p;
            break;
        case 2:
            c->r = p;
            c->g = v;
            c->b = t;
            break;

        case 3:
            c->r = p;
            c->g = q;
            c->b = v;
            break;
        case 4:
            c->r = t;
            c->g = p;
            c->b = v;
            break;
        case 5:
        default:
            c->r = v;
            c->g = p;
            c->b = q;
            break;
    }
}

void Helpers::RGBtoHSV(float& h, float& s, float& v, const PixelRGB* c)
{
    double min, max, delta;

    min = c->r < c->g ? c->r : c->g;
    min = min < c->b ? min : c->b;

    max = c->r > c->g ? c->r : c->g;
    max = max > c->b ? max : c->b;

    v = max / 255.0;
    delta = max - min;
    if (delta < 0.00001)
    {
        s = 0;
        h = 0;
        return;
    }
    if (max > 0.0) {
        s = (delta / max);
    }
    else {
        s = 0.0;
        h = 0.0;
        return;
    }
    if (c->r >= max)
        h = (c->g - c->b) / delta;
    else
        if (c->g >= max)
            h = 2.0 + (c->b - c->r) / delta;
        else
            h = 4.0 + (c->r - c->g) / delta;

    h *= 60.0;

    if (h < 0.0)
        h += 360.0;
}

void Helpers::Vector2DtoNormalColor(const Vector2D& vec, PixelRGB* ret)
{
    float mag = vec.Magnitude();
    if (mag < 0.001f)
    {
        Helpers::NormalMapDefaultColor(ret);
        return;
    }
    Vector2D vNorm = vec / mag;
    
    ret->r = (Uint8)(Lerp(0, 255, InverseLerp(-1.0f, 1.0f, vNorm[0])));
    ret->g = (Uint8)(Lerp(0, 255, InverseLerp(-1.0f, 1.0f, -vNorm[1]))); // SDL Y-Axis is inverted
    ret->b = 128;
}

void Helpers::Vector2DtoNormalColor(const Vector2D& vec, SDL_Color* ret)
{
    float mag = vec.Magnitude();
    if (mag < 0.001f)
    {
        Helpers::NormalMapDefaultColor(ret);
        return;
    }
    Vector2D vNorm = vec / mag;

    ret->r = (Uint8)(Lerp(0, 255, InverseLerp(-1.0f, 1.0f, vNorm[0])));
    ret->g = (Uint8)(Lerp(0, 255, InverseLerp(-1.0f, 1.0f, -vNorm[1]))); // SDL Y-Axis is inverted
    ret->b = 128;
    ret->a = 255;
}

void Helpers::Vector2DtoNormalColorHalf(const Vector2D& vec, PixelRGB* ret, bool posPolarity)
{
    float mag = vec.Magnitude();
    if (mag < 0.001f)
    {
        Helpers::NormalMapDefaultColor(ret);
        return;
    }
    Vector2D vNorm = vec / mag;

    if (vNorm[0] < 0)
        vNorm[1] *= -1;

    vNorm[0] = std::abs(vNorm[0]);

    ret->r = (Uint8)(Lerp(128, 255, vNorm[0]));
    ret->g = (Uint8)(Lerp(0, 255, InverseLerp(-1.0f, 1.0f, -vNorm[1])));
    ret->b = 128;
}

void Helpers::Vector2DtoNormalColorHalf(const Vector2D& vec, SDL_Color* ret, bool positivePolarity)
{
    float mag = vec.Magnitude();
    if (mag < 0.001f)
    {
        Helpers::NormalMapDefaultColor(ret);
        return;
    }
    Vector2D vNorm = vec / mag;

    if (vNorm[0] < 0)
        vNorm[1] *= -1;

    vNorm[0] = std::abs(vNorm[0]);

    // Cutting x-axis in half; y-axis is inverted (because of SDL) and still maintains
    // its full range. Therefore, y-axis must be remaped between -1 and 1 for its lerp
    // while the x-axis is naturally always positive and between 0-1.
    ret->r = (positivePolarity) ? (Uint8)(Lerp(128, 255, vNorm[0])) : (Uint8)(Lerp(128, 0, vNorm[0]));
    ret->g = (Uint8)(Lerp(0, 255, InverseLerp(-1.0f, 1.0f, ((positivePolarity) ? -vNorm[1] : vNorm[1]))));
    ret->b = 128;
    ret->a = 255;
}

Vector2D Helpers::HalfNormalColorToDirection(Uint8 r, Uint8 g, bool positivePolarity)
{
    r = std::max(128, (int)r);
    
    // X is always positive, Y can be negative (due to unclamped inverse lerp)
    return Vector2D((positivePolarity) ? InverseLerp(128.0f, 255.0f, (float)r) : InverseLerp(128.0f, 0.0f, (float)r), // x
                    -InverseLerp(128.0f, 255.0f, (float)g)).Get_Normalized();                                         // y
}

float Helpers::InverseLerp(float min, float max, float val)
{
    return (val - min) / (max - min);
}

float Helpers::Lerp(float min, float max, float t)
{
    return (min * (1.0f - t)) + (max * t);
}

void Helpers::LerpColor(const PixelRGB& c1, const PixelRGB& c2, float t, PixelRGB* ret)
{
    ret->r = (Uint8)(Lerp(c1.r, c2.r, t));
    ret->g = (Uint8)(Lerp(c1.g, c2.g, t));
    ret->b = (Uint8)(Lerp(c1.b, c2.b, t));
}

void Helpers::ComputeBarycentricCoordinates(const Vector2D& p, const Vector2D& a, const Vector2D& b, const Vector2D& c, 
    float& u, float& v, float& w)
{
    Vector2D v0 = b - a;
    Vector2D v1 = c - a;
    Vector2D v2 = p - a;
    double den = 1.0f / (v0[0] * v1[1] - v1[0] * v0[1]);
    v = (v2[0] * v1[1] - v1[0] * v2[1]) * den;
    w = (v0[0] * v2[1] - v2[0] * v0[1]) * den;
    u = 1.0f - v - w;
}

bool Helpers::PointTriangleIntersection(const Vector2D& s, const Vector2D& a, const Vector2D& b, const Vector2D& c)
{
    float u, v, w;
    Helpers::ComputeBarycentricCoordinates(s, a, b, c, u, v, w);

    return (u >= -0.013 && v >= -0.013 && w >= -0.013);
}

float Helpers::Clamp(float val, float min, float max)
{
    return std::max(min, std::min(val, max));
}

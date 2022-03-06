#ifndef VECTOR2D_H
#define VECTOR2D_H

// Sorry if using these constants is confusing, I like having these
// pre-defined in my other projects since it's technically faster.

// Rad to degrees = 180 / M_PI exact value,
// Deg to radians = M_PI / 180 exact value,
// TAU = 2 * PI exact value
#define  _USE_MATH_DEFINES
#define  RAD_TO_DEGREES 57.29577951308232087684
#define  DEG_TO_RADIANS 0.01745329251994329576
#define  TAU 6.28318530717958647692

#include <cmath>
#include <limits>
#include <float.h>

/*
 *  A class that represents a position, direction, etc. in 2D space. 
 *  Designed to simplify common mathematics that need to be done
 *  with vectors, like retrieving magnitude, normalizing, adding,
 *  subtracting, etc. Designed to be fully inlined and convenient.
 * 
 *  I tried to give this roughly as much functionality as something like Unity3D's vector
 *  classes to ensure that most possible needs of a vector class were met.
 */
class Vector2D
{
private:
	// 0 = x, 1 = y
	double data[2];

public:
	Vector2D();
	Vector2D(const double& x, const double& y);
	Vector2D(const Vector2D& copy);
	~Vector2D();

	// ---- Getters/Setters ---- //
	double Get_X() const;
	double Get_Y() const;

	void Set_X(const double& value);
	void Set_Y(const double& value);

	double& operator[] (const int& element);
	const double& operator[] (const int& element) const;

	// ---- Operator overloads used for calculations ---- //
	Vector2D& operator=  (const Vector2D& other);
	Vector2D& operator=  (const double& value);

	Vector2D& operator+= (const Vector2D& other);
	Vector2D& operator+= (const double& value);

	Vector2D& operator-= (const Vector2D& other);
	Vector2D& operator-= (const double& value);

	Vector2D& operator*= (const double& value);
	Vector2D& operator/= (const double& value);

	Vector2D operator+ (const Vector2D& other) const;
	Vector2D operator- (const Vector2D& other) const;
	Vector2D operator* (const double& value) const;
	Vector2D operator/ (const double& value) const;
	
	// Some friend operator funcs as well.
	// I'm implementing them here so they don't blend in with the other overloads!
	friend Vector2D operator* (const double& value, const Vector2D& other)
	{
		return other * value;
	}
	friend Vector2D operator- (const Vector2D& other)
	{
		return other * (-1.0);
	}

	friend bool operator== (const Vector2D& v1, const Vector2D& v2)
	{
		return (std::abs(v1[0] - v2[0]) < std::numeric_limits<double>::epsilon() ||
			std::abs(v1[1] - v2[1]) < std::numeric_limits<double>::epsilon());
	}

	// ---- Vector math functions and utilities ---- //
	static double CrossMagnitude(const Vector2D& v1, const Vector2D& v2);
	static double Dot(const Vector2D& v1, const Vector2D& v2);
	static double Distance(const Vector2D& v1, const Vector2D& v2);
	static double UnsignedAngle(const Vector2D& v1, const Vector2D& v2);
	static double SignedAngle(const Vector2D& v1, const Vector2D& v2);
	static Vector2D Lerp(const Vector2D& v1, const Vector2D& v2, double t);
	static Vector2D LerpUnclamped(const Vector2D& v1, const Vector2D& v2, const double& t);

	const double Magnitude() const;
	const double SqrMagnitude() const;

	// Gets clockwise are counter-clockwise perpendicular vector.
	// Not accounted for magnitude.
	const Vector2D PerpendicularCW() const;	
	const Vector2D PerpendicularCCW() const;

	void Normalize();
	Vector2D Get_Normalized() const;

	const double Angle() const;	// <- gets unsigned angle of this vector.
	const Vector2D Get_Rotated(const double& angleDeg);
	void Rotate(const double& angleDeg);
	void Set_Rotation(const double& angleDeg);
};

#endif
#ifndef VECTOR3D_H
#define VECTOR3D_H

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
 *  A class that represents a position, direction, etc. in 3D space.
 *  Designed to simplify common mathematics that need to be done
 *  with vectors, like retrieving magnitude, normalizing, adding,
 *  subtracting, etc. Designed to be fully inlined and convenient.
 *
 *  I tried to give this roughly as much functionality as something like Unity3D's vector
 *  class to ensure that most possible needs of a vector class were met.
 */
class Vector3D
{
private:

	// 0 = x, 
	// 1 = y, 
	// 2 = z
	double data[3];

public:

	Vector3D();
	Vector3D(const double& x, const double& y);
	Vector3D(const double& x, const double& y, const double& z);
	Vector3D(const Vector3D& copy);
	~Vector3D();

	// ---- Getters/Setters ---- //
	double Get_X() const;
	double Get_Y() const;
	double Get_Z() const;

	void Set_X(const double& value);
	void Set_Y(const double& value);
	void Set_Z(const double& value);

	double& operator[] (const int& element);
	const double& operator[] (const int& element) const;

	// ---- Operator overloads used for calculations ---- //
	Vector3D& operator=  (const Vector3D& other);
	Vector3D& operator=  (const double& value);

	Vector3D& operator+= (const Vector3D& other);
	Vector3D& operator+= (const double& value);

	Vector3D& operator-= (const Vector3D& other);
	Vector3D& operator-= (const double& value);

	Vector3D& operator*= (const double& value);
	Vector3D& operator/= (const double& value);

	Vector3D operator+ (const Vector3D& other) const;
	Vector3D operator- (const Vector3D& other) const;
	Vector3D operator* (const double& value) const;
	Vector3D operator/ (const double& value) const;

	// Some friend operator funcs as well.
	// I'm implementing them here so they don't blend in with the other overloads!
	friend Vector3D operator* (const double& value, const Vector3D& other)
	{
		return other * value;
	}
	friend Vector3D operator- (const Vector3D& other)
	{
		return other * (-1.0);
	}

	friend bool operator== (const Vector3D& v1, const Vector3D& v2)
	{
		return (std::abs(v1[0] - v2[0]) < std::numeric_limits<double>::epsilon() ||
			std::abs(v1[1] - v2[1]) < std::numeric_limits<double>::epsilon());
	}

	// ---- Vector math functions and utilities ---- //
	static Vector3D Cross(const Vector3D& v1, const Vector3D& v2);
	static double Dot(const Vector3D& v1, const Vector3D& v2);
	static double Distance(const Vector3D& v1, const Vector3D& v2);
	static double Angle(const Vector3D& v1, const Vector3D& v2);
	static Vector3D Lerp(const Vector3D& v1, const Vector3D& v2, double t);
	static Vector3D LerpUnclamped(const Vector3D& v1, const Vector3D& v2, const double& t);

	const double Magnitude() const;
	const double SqrMagnitude() const;

	void Normalize();
	Vector3D Get_Normalized();
};
#endif
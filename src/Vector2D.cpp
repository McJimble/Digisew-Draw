#include "Vector2D.h"

Vector2D::Vector2D()
{
	data[0] = 0.0;
	data[1] = 0.0;
}

Vector2D::Vector2D(const double& x, const double& y)
{
	data[0] = x;
	data[1] = y;
}

Vector2D::Vector2D(const Vector2D& copy)
{
	this->data[0] = copy.data[0];
	this->data[1] = copy.data[1];
}

Vector2D::~Vector2D()
{

}

/*
*  Simple getters/setters. Can be done explicity or via array access.
*  For speed, array access doesn't check if in bounds. Use with caution!
*/
double Vector2D::Get_X() const { return data[0]; }
double Vector2D::Get_Y() const { return data[1]; }

void Vector2D::Set_X(const double& value) { data[0] = value; }
void Vector2D::Set_Y(const double& value) { data[1] = value; }

double& Vector2D::operator[](const int& element) { return data[element]; }
const double& Vector2D::operator[](const int& element) const { return data[element]; }

/*
*  Operator overload methods for component-wise vector math.
*  Can modify this instance or create new ones.
*/

Vector2D& Vector2D::operator= (const Vector2D& other)
{
	this->data[0] = other.data[0];
	this->data[1] = other.data[1];
	return *this;
}

Vector2D& Vector2D::operator= (const double& value)
{
	this->data[0] = value;
	this->data[1] = value;
	return *this;
}

Vector2D& Vector2D::operator+= (const Vector2D& other)
{
	this->data[0] += other.data[0];
	this->data[1] += other.data[1];
	return *this;
}

Vector2D& Vector2D::operator+= (const double& value)
{
	this->data[0] += value;
	this->data[1] += value;
	return *this;
}

Vector2D& Vector2D::operator-= (const Vector2D& other)
{
	this->data[0] -= other.data[0];
	this->data[1] -= other.data[1];
	return *this;
}

Vector2D& Vector2D::operator-= (const double& value)
{
	this->data[0] -= value;
	this->data[1] -= value;
	return *this;
}

Vector2D& Vector2D::operator*= (const double& value)
{
	this->data[0] *= value;
	this->data[1] *= value;
	return *this;
}

Vector2D& Vector2D::operator/= (const double& value)
{
	this->data[0] /= value;
	this->data[1] /= value;
	return *this;
}

Vector2D Vector2D::operator+ (const Vector2D& other) const
{
	return Vector2D(this->data[0] + other.data[0], this->data[1] + other.data[1]);
}

Vector2D Vector2D::operator- (const Vector2D& other) const
{
	return Vector2D(this->data[0] - other.data[0], this->data[1] - other.data[1]);
}

Vector2D Vector2D::operator* (const double& value) const
{
	return Vector2D(this->data[0] * value, this->data[1] * value);
}

Vector2D Vector2D::operator/ (const double& value) const
{
	return Vector2D(this->data[0] / value, this->data[1] / value);
}

/*
*  Vector math functions and utilities
*/

// Cross-Product might seem silly to use in 2D, but it's magnitude, and the
// relationship with it's sign and the direction of rotation to follow if
// v1 were desired to rotate towards v2 (therefore we only return the magnitude)
double Vector2D::CrossMagnitude(const Vector2D& v1, const Vector2D& v2)
{
	return (v1[0] * v2[1]) - (v1[1] * v2[0]);
}

double Vector2D::Dot(const Vector2D& v1, const Vector2D& v2)
{
	return (v1[0] * v2[0]) + (v1[1] * v2[1]);
}

double Vector2D::Distance(const Vector2D& v1, const Vector2D& v2)
{
	return (v2 - v1).Magnitude();
}

// Long calculation because converting from counter clockwise
// to clockwise rotation. Have to negate unsigned angle (found by using negative vectors and
// adding pi or 180 deg. to it), then add 90 degrees ( pi / 2 ),
// THEN mod with 360 degrees (2 radians). Finally convert from radians to degrees.
double Vector2D::UnsignedAngle(const Vector2D& v1, const Vector2D& v2)
{
	double nonCW = -std::atan2(-Vector2D::CrossMagnitude(v1, v2), -Vector2D::Dot(v1, v2)) + M_PI;
	double temp = std::fmod(nonCW, TAU) * RAD_TO_DEGREES;
	return temp;
}

// Negating at the end because atan2 returns counterclockwise rotation,
// but SDL uses clockwise.
double Vector2D::SignedAngle(const Vector2D& v1, const Vector2D& v2)
{
	return (-std::atan2(Vector2D::CrossMagnitude(v1, v2), Vector2D::Dot(v1, v2)) * RAD_TO_DEGREES);
}

Vector2D Vector2D::Lerp(const Vector2D& v1, const Vector2D& v2, double t)
{
	t = (t > 1.0) ? 1.0 : t;
	t = (t < 0.0) ? 0.0 : t;
	return (v1 + (v2 - v1) * t);
}

Vector2D Vector2D::LerpUnclamped(const Vector2D& v1, const Vector2D& v2, const double& t)
{
	return (v1 + (v2 - v1) * t);
}

const double Vector2D::Magnitude() const
{
	return std::sqrt((data[0] * data[0]) + (data[1] * data[1]));
}

// This is useful for distance checks where all we care about is
// whether something is in range at all and not how much in range it is.
const double Vector2D::SqrMagnitude() const
{
	return (data[0] * data[0]) + (data[1] * data[1]);
}

const Vector2D Vector2D::PerpendicularCW() const
{
	return Vector2D(data[1], -data[0]);
}

const Vector2D Vector2D::PerpendicularCCW() const
{
	return Vector2D(-data[1], data[0]);
}

void Vector2D::Normalize()
{
	const double magnitude = this->Magnitude();
	data[0] = (magnitude > 0.0000001) ? data[0] / magnitude : 0;
	data[1] = (magnitude > 0.0000001) ? data[1] / magnitude : 0;
}

Vector2D Vector2D::Get_Normalized() const
{
	Vector2D n = Vector2D(data[0], data[1]);
	n.Normalize();
	return n;
}

// Gets unsigned angle by just using UnsignedAngle function but with 0,0 as the
// other vector, since 0,0 represents 0 degrees in our cases.
const double Vector2D::Angle() const
{
	return Vector2D::UnsignedAngle(*this, Vector2D(1, 0));
}

const Vector2D Vector2D::Get_Rotated(const double& angleDeg)
{
	Vector2D rot = Vector2D(data[0], data[1]);
	rot.Rotate(angleDeg);
	return rot;
}

void Vector2D::Rotate(const double& angleDeg)
{
	const double angleRad = angleDeg * DEG_TO_RADIANS;
	double oldX = data[0];
	double oldY = data[1];

	data[0] = oldX * std::cos(angleRad) - oldY * std::sin(angleRad);
	data[1] = oldX * std::sin(angleRad) + oldY * std::cos(angleRad);
}

void Vector2D::Set_Rotation(const double& angleDeg)
{
	const double angleRad = angleDeg * DEG_TO_RADIANS;
	data[0] = std::cos(angleRad);
	data[1] = std::sin(angleRad);
}
#include "Vector3D.h"

Vector3D::Vector3D()
{
	data[0] = 0.0;
	data[1] = 0.0;
	data[2] = 0.0;
}

Vector3D::Vector3D(const double& x, const double& y)
{
	data[0] = x;
	data[1] = y;
	data[2] = 0.0;
}

Vector3D::Vector3D(const double& x, const double& y, const double& z)
{
	data[0] = x;
	data[1] = y;
	data[2] = z;
}


Vector3D::Vector3D(const Vector3D& copy)
{
	this->data[0] = copy.data[0];
	this->data[1] = copy.data[1];
	this->data[2] = copy.data[2];
}

Vector3D::~Vector3D()
{

}

/*
*  Simple getters/setters. Can be done explicity or via array access.
*  For speed, array access doesn't check if in bounds. Use with caution!
*/
double Vector3D::Get_X() const { return data[0]; }
double Vector3D::Get_Y() const { return data[1]; }
double Vector3D::Get_Z() const { return data[2]; }

void Vector3D::Set_X(const double& value) { data[0] = value; }
void Vector3D::Set_Y(const double& value) { data[1] = value; }
void Vector3D::Set_Z(const double& value) { data[2] = value; }

double& Vector3D::operator[](const int& element) { return data[element]; }
const double& Vector3D::operator[](const int& element) const { return data[element]; }

/*
*  Operator overload methods for component-wise vector math.
*  These can modify this instance or be used to create new ones.
*/

Vector3D& Vector3D::operator= (const Vector3D& other)
{
	this->data[0] = other.data[0];
	this->data[1] = other.data[1];
	this->data[2] = other.data[2];
	return *this;
}

Vector3D& Vector3D::operator= (const double& value)
{
	this->data[0] = value;
	this->data[1] = value;
	this->data[2] = value;
	return *this;
}

Vector3D& Vector3D::operator+= (const Vector3D& other)
{
	this->data[0] += other.data[0];
	this->data[1] += other.data[1];
	this->data[2] += other.data[2];
	return *this;
}

Vector3D& Vector3D::operator+= (const double& value)
{
	this->data[0] += value;
	this->data[1] += value;
	this->data[2] += value;
	return *this;
}

Vector3D& Vector3D::operator-= (const Vector3D& other)
{
	this->data[0] -= other.data[0];
	this->data[1] -= other.data[1];
	this->data[2] -= other.data[2];
	return *this;
}

Vector3D& Vector3D::operator-= (const double& value)
{
	this->data[0] -= value;
	this->data[1] -= value;
	this->data[2] -= value;
	return *this;
}

Vector3D& Vector3D::operator*= (const double& value)
{
	this->data[0] *= value;
	this->data[1] *= value;
	this->data[2] *= value;
	return *this;
}

Vector3D& Vector3D::operator/= (const double& value)
{
	this->data[0] /= value;
	this->data[1] /= value;
	this->data[2] /= value;
	return *this;
}

Vector3D Vector3D::operator+ (const Vector3D& other) const
{
	return Vector3D(this->data[0] + other.data[0], this->data[1] + other.data[1], this->data[2] + other.data[2]);
}

Vector3D Vector3D::operator- (const Vector3D& other) const
{
	return Vector3D(this->data[0] - other.data[0], this->data[1] - other.data[1], this->data[2] - other.data[2]);
}

Vector3D Vector3D::operator* (const double& value) const
{
	return Vector3D(this->data[0] * value, this->data[1] * value, this->data[2] * value);
}

Vector3D Vector3D::operator/ (const double& value) const
{
	return Vector3D(this->data[0] / value, this->data[1] / value, this->data[2] / value);
}

/*
*  Vector math functions and utilities
*/

Vector3D Vector3D::Cross(const Vector3D& v1, const Vector3D& v2)
{
	return Vector3D(v1[1] * v2[2] - v1[2] * v2[1],
					v1[2] * v2[0] - v1[0] - v2[2],
					v1[0] * v2[1] - v1[1] - v2[0]);
}

double Vector3D::Dot(const Vector3D& v1, const Vector3D& v2)
{
	return (v1[0] * v2[0]) + (v1[1] * v2[1]);
}

double Vector3D::Distance(const Vector3D& v1, const Vector3D& v2)
{
	return (v2 - v1).Magnitude();
}

double Vector3D::Angle(const Vector3D& v1, const Vector3D& v2)
{
	return std::acos(Dot(v1, v2) / (v1.Magnitude() * v2.Magnitude()));
}

Vector3D Vector3D::Lerp(const Vector3D& v1, const Vector3D& v2, double t)
{
	t = (t > 1.0) ? 1.0 : t;
	t = (t < 0.0) ? 0.0 : t;
	return (v1 + (v2 - v1) * t);
}

Vector3D Vector3D::LerpUnclamped(const Vector3D& v1, const Vector3D& v2, const double& t)
{
	return (v1 + (v2 - v1) * t);
}

const double Vector3D::Magnitude() const
{
	return std::sqrt((data[0] * data[0]) + (data[1] * data[1]) + (data[2] * data[2]));
}

// This is useful for distance checks where all we care about is
// whether something is in range at all and not how much in range it is.
const double Vector3D::SqrMagnitude() const
{
	return (data[0] * data[0]) + (data[1] * data[1]) + (data[2] * data[2]);
}

void Vector3D::Normalize()
{
	const double magnitude = this->Magnitude();
	data[0] = (magnitude > 0.0000001) ? data[0] / magnitude : 0;
	data[1] = (magnitude > 0.0000001) ? data[1] / magnitude : 0;
	data[2] = (magnitude > 0.0000001) ? data[2] / magnitude : 0;
}

Vector3D Vector3D::Get_Normalized()
{
	Vector3D n = Vector3D(data[0], data[1], data[2]);
	n.Normalize();
	return n;
}

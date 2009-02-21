#pragma once

class Vector3
{
public:
	float x;
	float y;
	float z;

	Vector3(void)
	{
	}

	Vector3(const float x, const float y, const float z)
		: x(x), y(y), z(z)
	{
	}

	Vector3(const Vector3 &v)
		: x(v.x), y(v.y), z(v.z)
	{
	}

	~Vector3(void)
	{
	}

	float &operator [](int index)
	{
		return (&x)[index];
	}

	const float &operator [](int index) const
	{
		return (&x)[index];
	}

	friend const Vector3 operator+(const Vector3 &v1, const Vector3 &v2)
	{
		return Vector3(v1.x + v2.x, v1.y + v2.y, v1.z + v2.z);
	}

	Vector3 &operator+=(const Vector3 &v)
	{
		*this = *this + v;
		return *this;
	}

	friend const Vector3 operator-(const Vector3 &v)
	{
		return Vector3(-v.x, -v.y, -v.z);
	}

	friend const Vector3 operator-(const Vector3 &v1, const Vector3 &v2)
	{
		return Vector3(v1.x - v2.x, v1.y - v2.y, v1.z - v2.z);
	}

	Vector3 &operator-=(const Vector3 &v)
	{
		*this = *this - v;
		return *this;
	}

	friend const Vector3 operator*(const Vector3 &v1, const Vector3 &v2)
	{
		return Vector3(v1.x * v2.x, v1.y * v2.y, v1.z * v2.z);
	}

	Vector3 &operator*=(const Vector3 &v)
	{
		*this = *this * v;
		return *this;
	}

	friend const Vector3 operator/(const Vector3 &v1, const Vector3 &v2)
	{
		return Vector3(v1.x / v2.x, v1.y / v2.y, v1.z / v2.z);
	}

	Vector3 &operator/=(const Vector3 &v)
	{
		*this = *this / v;
		return *this;
	}

	friend const Vector3 operator*(const Vector3 &v, const float s)
	{
		return Vector3(v.x * s, v.y * s, v.z * s);
	}

	friend const Vector3 operator*(const float s, const Vector3 &v)
	{
		return Vector3(v.x * s, v.y * s, v.z * s);
	}

	Vector3 &operator*=(const float s)
	{
		*this = *this * s;
		return *this;
	}

	friend const Vector3 operator/(const Vector3 &v, const float s)
	{
		return Vector3(v.x / s, v.y / s, v.z / s);
	}

	Vector3 &operator/=(const float s)
	{
		*this = *this / s;
		return *this;
	}

	float Dot(const Vector3 &v) const
	{
		return x * v.x + y * v.y + z * v.z;
	}

	Vector3 Cross(const Vector3 &v) const
	{
		return Vector3(y * v.z - z * v.y, z * v.x - x * v.z, x * v.y - y * v.x);
	}

	float LengthSq(void) const
	{
		return x * x + y * y + z * z;
	}

	float Length(void) const
	{
		return sqrtf(LengthSq());
	}

	float DistSq(const Vector3 &v) const
	{
		return (v.x - x) * (v.x - x) + (v.y - y) * (v.y - y) + (v.z - z) * (v.z - z);
	}

	float Dist(const Vector3 &v) const
	{
		return sqrtf(DistSq(v));
	}
};

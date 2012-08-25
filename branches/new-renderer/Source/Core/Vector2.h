#pragma once

class Vector2
{
public:
	float x;
	float y;

	Vector2(void)
	{
	}

	Vector2(const float x, const float y)
		: x(x), y(y)
	{
	}

	Vector2(const Vector2 &v)
		: x(v.x), y(v.y)
	{
	}

	~Vector2(void)
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

	friend const Vector2 operator+(const Vector2 &v1, const Vector2 &v2)
	{
		return Vector2(v1.x + v2.x, v1.y + v2.y);
	}

	Vector2 &operator+=(const Vector2 &v)
	{
		*this = *this + v;
		return *this;
	}

	friend const Vector2 operator-(const Vector2 &v)
	{
		return Vector2(-v.x, -v.y);
	}

	friend const Vector2 operator-(const Vector2 &v1, const Vector2 &v2)
	{
		return Vector2(v1.x - v2.x, v1.y - v2.y);
	}

	Vector2 &operator-=(const Vector2 &v)
	{
		*this = *this - v;
		return *this;
	}

	friend const Vector2 operator*(const Vector2 &v1, const Vector2 &v2)
	{
		return Vector2(v1.x * v2.x, v1.y * v2.y);
	}

	Vector2 &operator*=(const Vector2 &v)
	{
		*this = *this * v;
		return *this;
	}

	friend const Vector2 operator*(const Vector2 &v, const float s)
	{
		return Vector2(v.x * s, v.y * s);
	}

	friend const Vector2 operator*(const float s, const Vector2 &v)
	{
		return Vector2(v.x * s, v.y * s);
	}

	Vector2 &operator*=(const float s)
	{
		*this = *this * s;
		return *this;
	}

	friend const Vector2 operator/(const Vector2 &v1, const Vector2 &v2)
	{
		return Vector2(v1.x / v2.x, v1.y / v2.y);
	}

	Vector2 &operator/=(const Vector2 &v)
	{
		*this = *this / v;
		return *this;
	}

	friend const Vector2 operator/(const Vector2 &v, const float s)
	{
		return Vector2(v.x / s, v.y / s);
	}

	Vector2 &operator/=(const float s)
	{
		*this = *this / s;
		return *this;
	}

	float Dot(const Vector2 &v) const
	{
		return x * v.x + y * v.y;
	}

	float Cross(const Vector2 &v) const
	{
		return x * v.y - y * v.x;
	}

	float LengthSq(void) const
	{
		return x * x + y * y;
	}

	float Length(void) const
	{
		return sqrtf(LengthSq());
	}

	float DistSq(const Vector2 &v) const
	{
		return (v.x - x) * (v.x - x) + (v.y - y) * (v.y - y);
	}

	float Dist(const Vector2 &v) const
	{
		return sqrtf(DistSq(v));
	}
};

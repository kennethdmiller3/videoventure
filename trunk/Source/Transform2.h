#pragma once

#include "Vector2.h"
#include "Matrix2.h"

class Transform2
{
public:
	float a;
	Vector2 p;

public:
	Transform2()
	{
	}

	Transform2(float aAngle, const Vector2 &aPosit)
		: a(aAngle), p(aPosit)
	{
	}

	Transform2(const Matrix2 &aM)
		: a(aM.Angle()), p(aM.p)
	{
	}

	~Transform2()
	{
	}

	Vector2 Transform(const Vector2 &v) const
	{
		return Vector2(
			p.x + v.x * cosf(a) - v.y * sinf(a),
			p.y + v.x * sinf(a) + v.y * cosf(a)
			);
	}

	Vector2 Untransform(const Vector2 &v) const
	{
		return Unrotate(v - p);
	}

	Vector2 Rotate(const Vector2 &v) const
	{
		return Vector2(
			v.x * cosf(a) - v.y * sinf(a),
			v.x * sinf(a) + v.y * cosf(a)
			);
	}
	
	Vector2 Unrotate(const Vector2 &v) const
	{
		return Vector2(
			v.x * cosf(a) + v.y * sinf(a),
			v.x * -sinf(a) + v.y * cosf(a)
			);
	}

	float Angle(void) const
	{
		return a;
	}

	friend const Transform2 operator*(const Transform2 &t1, const Transform2 &t2)
	{
		return Transform2(
			t2.a + t1.a,
			Vector2(
				t2.p.x + t1.p.x * cosf(t2.a) - t1.p.y * sinf(t2.a),
				t2.p.y + t1.p.x * sinf(t2.a) + t1.p.y * cosf(t2.a)
				)
			);
	}

	Transform2 &operator*=(const Transform2 &m)
	{
		*this = *this * m;
		return *this;
	}

	friend Transform2 Lerp(const Transform2 &t1, const Transform2 &t2, float s)
	{
		return Transform2(t1.a * (1 - s) + t2.a * s, t1.p * (1-s) + t2.p * s);
	}

	const Transform2 Inverse(void) const
	{
		return Transform2(-a, -Unrotate(p));
	}

	operator const Matrix2() const
	{
		return Matrix2(a, p);
	}

};
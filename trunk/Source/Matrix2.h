#pragma once

class Matrix2
{
public:
	Vector2 x;
	Vector2 y;
	Vector2 p;

public:

	Matrix2(void)
	{
	}

	Matrix2(const Vector2 &aX, const Vector2 &aY, const Vector2 &aP)
		: x(aX), y(aY), p(aP)
	{
	}

	~Matrix2(void)
	{
	}

	Vector2 Transform(const Vector2 &v)
	{
		return x * v.x + y * v.y + p;
	}

	Vector2 Rotate(const Vector2 &v)
	{
		return x* v.x + y * v.y;
	}

	friend const Matrix2 operator*(const Matrix2 &a, const Matrix2 &b)
	{
		return Matrix2(
			a.x.x * b.x + a.x.y * b.y,
			a.y.x * b.x	+ a.y.y * b.y,
			a.p.x * b.x + a.p.y * b.y + b.p
			);
	}

	Matrix2 &operator*=(const Matrix2 &m)
	{
		*this = *this * m;
		return *this;
	}
};

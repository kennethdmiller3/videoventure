#pragma once

struct Color4
{
	float r;
	float g;
	float b;
	float a;

	Color4()
	{
	}

	Color4(float r, float g, float b, float a = 1.0f)
		: r(r), g(g), b(b), a(a)
	{
	}

	operator float *()
	{
		return &r;
	}

	operator const float *() const
	{
		return &r;
	}

	float &operator [](int index)
	{
		return (&r)[index];
	}

	const float &operator [](int index) const
	{
		return (&r)[index];
	}

	friend const Color4 operator+(const Color4 &c1, const Color4 &c2)
	{
		return Color4(c1.r+c2.r, c1.g+c2.g, c1.b+c2.b, c1.a+c2.a);
	}

	Color4 &operator+=(const Color4 &c)
	{
		*this = *this + c;
		return *this;
	}

	friend const Color4 operator-(const Color4 &c)
	{
		return Color4(-c.r, -c.g, -c.b, -c.a);
	}

	friend const Color4 operator-(const Color4 &c1, const Color4 &c2)
	{
		return Color4(c1.r-c2.r, c1.g-c2.g, c1.b-c2.b, c1.a-c2.a);
	}

	Color4 &operator-=(const Color4 &c)
	{
		*this = *this - c;
		return *this;
	}

	friend const Color4 operator*(const Color4 &c1, const Color4 &c2)
	{
		return Color4(c1.r*c2.r, c1.g*c2.g, c1.b*c2.b, c1.a*c2.a);
	}

	Color4 &operator*=(const Color4 &c)
	{
		*this = *this * c;
		return *this;
	}

	friend const Color4 operator*(const Color4 &c, const float s)
	{
		return Color4(c.r*s, c.g*s, c.b*s, c.a*s);
	}

	friend const Color4 operator*(const float s, const Color4 &c)
	{
		return Color4(c.r*s, c.g*s, c.b*s, c.a*s);
	}

	Color4 &operator*=(const float s)
	{
		*this = *this * s;
		return *this;
	}

	friend const Color4 operator/(const Color4 &c1, const Color4 &c2)
	{
		return Color4(c1.r/c2.r, c1.g/c2.g, c1.b/c2.b, c1.a/c2.a);
	}

	Color4 &operator/=(const Color4 &c)
	{
		*this = *this / c;
		return *this;
	}

	friend const Color4 operator/(const Color4 &c, const float s)
	{
		return Color4(c.r/s, c.g/s, c.b/s, c.a/s);
	}

	Color4 &operator/=(const float s)
	{
		*this = *this / s;
		return *this;
	}

};

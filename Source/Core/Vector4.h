#pragma once

class Vector4
{
public:
	float x;
	float y;
	float z;
	float w;

	Vector4()
	{
	}
	
	Vector4(const float x, const float y = 0, const float z = 0, const float w = 1)
		: x(x), y(y), z(z), w(w)
	{
	}

	Vector4(const Vector2 &v, const float z = 0, const float w = 1)
		: x(v.x), y(v.y), z(z),w(w)
	{
	}

	Vector4(const Vector3 &v, const float w = 1)
		: x(v.x), y(v.y), z(v.z),w(w)
	{
	}

	operator float *()
	{
		return &x;
	}

	operator const float *() const
	{
		return &x;
	}

	float &operator [](int index)
	{
		return (&x)[index];
	}

	const float &operator [](int index) const
	{
		return (&x)[index];
	}

	friend const Vector4 operator+(const Vector4 &a, const Vector4 &b)
	{
		return Vector4(a.x+b.x, a.y+b.y, a.z+b.z, a.w+b.w);
	}

	Vector4 &operator+=(const Vector4 &v)
	{
		*this = *this + v;
		return *this;
	}

	friend const Vector4 operator-(const Vector4 &v)
	{
		return Vector4(-v.x, -v.y, -v.z, -v.w);
	}

	friend const Vector4 operator-(const Vector4 &a, const Vector4 &b)
	{
		return Vector4(a.x-b.x, a.y-b.y, a.z-b.z, a.w-b.w);
	}

	Vector4 &operator-=(const Vector4 &v)
	{
		*this = *this - v;
		return *this;
	}

	friend const Vector4 operator*(const Vector4 &a, const Vector4 &b)
	{
		return Vector4(a.x*b.x, a.y*b.y, a.z*b.z, a.w*b.w);
	}

	Vector4 &operator*=(const Vector4 &v)
	{
		*this = *this * v;
		return *this;
	}

	friend const Vector4 operator*(const Vector4 &v, const float s)
	{
		return Vector4(v.x * s, v.y * s, v.z * s, v.w * s);
	}

	friend const Vector4 operator*(const float s, const Vector4 &v)
	{
		return Vector4(v.x * s, v.y * s, v.z * s, v.w * s);
	}

	Vector4 &operator*=(const float s)
	{
		*this = *this * s;
		return *this;
	}

	friend const Vector4 operator/(const Vector4 &a, const Vector4 &b)
	{
		return Vector4(a.x/b.x, a.y/b.y, a.z/b.z, a.w/b.w);
	}

	Vector4 &operator/=(const Vector4 &v)
	{
		*this = *this / v;
		return *this;
	}

	float Dot(const Vector4 &v) const
	{
		return x * v.x + y * v.y + z * v.z + w * v.w;
	}

	Vector4 Cross(const Vector4 &v) const
	{
		return Vector4(y * v.z - z * v.y, z * v.x - x * v.z, x * v.y - y * v.x, 1);
	}

	float LengthSq(void) const
	{
		return x * x + y * y + z * z + w * w;
	}

	float Length(void) const
	{
		return sqrtf(LengthSq());
	}

	float DistSq(const Vector4 &v) const
	{
		return (v.x - x) * (v.x - x) + (v.y - y) * (v.y - y) + (v.z - z) * (v.z - z) + (v.w - w) * (v.w - w);
	}

	float Dist(const Vector4 &v) const
	{
		return sqrtf(DistSq(v));
	}
};


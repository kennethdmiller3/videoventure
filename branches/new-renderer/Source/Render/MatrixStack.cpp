#include "StdAfx.h"
#include "MatrixStack.h"

//
// MATRIX STACK
//

#define MATRIX_STACK_SSE

// 4x4 matrix
struct Matrix4
{
#ifdef MATRIX_STACK_SSE
	__m128 m[4];
#else
	float m[4][4];
#endif

	static const Matrix4 Identity;
};

// matrix multiply
Matrix4 operator *(const Matrix4 &a, const Matrix4 &b)
{
#ifdef MATRIX_STACK_SSE
	Matrix4 o = {};
	for (register unsigned char i = 0; i < 4; ++i)
		o.m[i] = _mm_add_ps(
			_mm_add_ps(
				_mm_add_ps(
					_mm_mul_ps(a.m[0], _mm_shuffle_ps(b.m[i], b.m[i], _MM_SHUFFLE(0, 0, 0, 0))),
					_mm_mul_ps(a.m[1], _mm_shuffle_ps(b.m[i], b.m[i], _MM_SHUFFLE(1, 1, 1, 1)))
				),
				_mm_mul_ps(a.m[2], _mm_shuffle_ps(b.m[i], b.m[i], _MM_SHUFFLE(2, 2, 2, 2)))
			),
			_mm_mul_ps(a.m[3], _mm_shuffle_ps(b.m[i], b.m[i], _MM_SHUFFLE(3, 3, 3, 3)))
		);
#else
	Matrix4 o;
	o.m[0][0] = a.m[0][0] * b.m[0][0] + a.m[0][1] * b.m[1][0] + a.m[0][2] * b.m[2][0] + a.m[0][3] * b.m[3][0];
	o.m[0][1] = a.m[0][0] * b.m[0][1] + a.m[0][1] * b.m[1][1] + a.m[0][2] * b.m[2][1] + a.m[0][3] * b.m[3][1];
	o.m[0][2] = a.m[0][0] * b.m[0][2] + a.m[0][1] * b.m[1][2] + a.m[0][2] * b.m[2][2] + a.m[0][3] * b.m[3][2];
	o.m[0][3] = a.m[0][0] * b.m[0][3] + a.m[0][1] * b.m[1][3] + a.m[0][2] * b.m[2][3] + a.m[0][3] * b.m[3][3];
	o.m[1][0] = a.m[1][0] * b.m[0][0] + a.m[1][1] * b.m[1][0] + a.m[1][2] * b.m[2][0] + a.m[1][3] * b.m[3][0];
	o.m[1][1] = a.m[1][0] * b.m[0][1] + a.m[1][1] * b.m[1][1] + a.m[1][2] * b.m[2][1] + a.m[1][3] * b.m[3][1];
	o.m[1][2] = a.m[1][0] * b.m[0][2] + a.m[1][1] * b.m[1][2] + a.m[1][2] * b.m[2][2] + a.m[1][3] * b.m[3][2];
	o.m[1][3] = a.m[1][0] * b.m[0][3] + a.m[1][1] * b.m[1][3] + a.m[1][2] * b.m[2][3] + a.m[1][3] * b.m[3][3];
	o.m[2][0] = a.m[2][0] * b.m[0][0] + a.m[2][1] * b.m[1][0] + a.m[2][2] * b.m[2][0] + a.m[2][3] * b.m[3][0];
	o.m[2][1] = a.m[2][0] * b.m[0][1] + a.m[2][1] * b.m[1][1] + a.m[2][2] * b.m[2][1] + a.m[2][3] * b.m[3][1];
	o.m[2][2] = a.m[2][0] * b.m[0][2] + a.m[2][1] * b.m[1][2] + a.m[2][2] * b.m[2][2] + a.m[2][3] * b.m[3][2];
	o.m[2][3] = a.m[2][0] * b.m[0][3] + a.m[2][1] * b.m[1][3] + a.m[2][2] * b.m[2][3] + a.m[2][3] * b.m[3][3];
	o.m[3][0] = a.m[3][0] * b.m[0][0] + a.m[3][1] * b.m[1][0] + a.m[3][2] * b.m[2][0] + a.m[3][3] * b.m[3][0];
	o.m[3][1] = a.m[3][0] * b.m[0][1] + a.m[3][1] * b.m[1][1] + a.m[3][2] * b.m[2][1] + a.m[3][3] * b.m[3][1];
	o.m[3][2] = a.m[3][0] * b.m[0][2] + a.m[3][1] * b.m[1][2] + a.m[3][2] * b.m[2][2] + a.m[3][3] * b.m[3][2];
	o.m[3][3] = a.m[3][0] * b.m[0][3] + a.m[3][1] * b.m[1][3] + a.m[3][2] * b.m[2][3] + a.m[3][3] * b.m[3][3];
#endif
	return o;
}

// identity matrix;
const Matrix4 Matrix4::Identity = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };

// projection matrix
static Matrix4 sProjMatrix;

// matrix stack
static const int sStackEntries = 64;
static Matrix4 sStackMatrix[sStackEntries];
static int sStackTop;


//
// IDENTITY MATRIX OPERATIONS
//

const float *IdentityGet(void)
{
	return Matrix4::Identity.m->m128_f32;
}


//
// PROJECTION MATRIX OPERATIONS
//

void ProjectionLoad(const float *values)
{
	memcpy(&sProjMatrix, values, sizeof(Matrix4));
}

const float *ProjectionGet(void)
{
	return sProjMatrix.m->m128_f32;
}


//
// MATRIX STACK OPERATIONS
//

// initialize the stack
void StackInit(void)
{
	sStackTop = 0;
	StackIdentity();
#ifdef DEBUG
	memset(sStackMatrix+1, 0, sizeof(Matrix4) * (sStackEntries - 1));
#endif
}

// get the current matrix
const float *StackGet(void)
{
	return sStackMatrix[sStackTop].m->m128_f32;
}

// push the current entry
void StackPush(void)
{
	++sStackTop;
	sStackMatrix[sStackTop] = sStackMatrix[sStackTop-1];
}

// pop the current entry
void StackPop(void)
{
#ifdef DEBUG
	memset(sStackMatrix + sStackTop, 0, sizeof(Matrix4));
#endif
	--sStackTop;
}

// load an identity matrix
void StackIdentity(void)
{
	Matrix4 &m = sStackMatrix[sStackTop];
	m = Matrix4::Identity;
}

// load a matrix
void StackLoad(const float *values)
{
	Matrix4 &m = sStackMatrix[sStackTop];
	memcpy(&m, &values, sizeof(Matrix4));
}

// multiply by a matrix
void StackMult(const float *values)
{
	Matrix4 &m = sStackMatrix[sStackTop];
	Matrix4 b;
	memcpy(&b, &values, sizeof(Matrix4));
	m = m * b;
}

// apply rotation
void StackRotate(const float value)
{
	Matrix4 &m = sStackMatrix[sStackTop];
	//const Matrix4 r = 
	//{ 
	//	c, s, 0, 0,
	//	-s, c, 0, 0,
	//	0, 0, 1, 0,
	//	0, 0, 0, 1
	//};
	//m = m * r;
	const Matrix4 o = m;
#ifdef MATRIX_STACK_SSE
	const __m128 s(_mm_set_ps1(sinf(value))), c(_mm_set_ps1(cosf(value)));
	m.m[0] = _mm_add_ps(_mm_mul_ps(o.m[0], c), _mm_mul_ps(o.m[1], s));
	m.m[1] = _mm_sub_ps(_mm_mul_ps(o.m[1], c), _mm_mul_ps(o.m[0], s));
	m.m[2] = o.m[2];
	m.m[3] = o.m[3];
#else
	const float s(sinf(value*float(M_PI/180.0f))), c(cosf(value*float(M_PI/180.0f)));
	m.m[0][0] = o.m[0][0] * c + o.m[1][0] * s;
	m.m[0][1] = o.m[0][1] * c + o.m[1][1] * s;
	m.m[0][2] = o.m[0][2] * c + o.m[1][2] * s;
	m.m[1][0] = o.m[0][0] * -s + o.m[1][0] * c;
	m.m[1][1] = o.m[0][1] * -s + o.m[1][1] * c;
	m.m[1][2] = o.m[0][2] * -s + o.m[1][2] * c;
	m.m[2][0] = o.m[2][0];
	m.m[2][1] = o.m[2][1];
	m.m[2][2] = o.m[2][2];
	m.m[3][0] = o.m[3][0];
	m.m[3][1] = o.m[3][1];
	m.m[3][2] = o.m[3][2];
#endif
}

// apply scale
void StackScale(const __m128 value)
{
	//glScalef(value.m128_f32[0], value.m128_f32[1], value.m128_f32[2]);
	Matrix4 &m = sStackMatrix[sStackTop];
#ifdef MATRIX_STACK_SSE
	m.m[0] = _mm_mul_ps(m.m[0], _mm_shuffle_ps(value, value, _MM_SHUFFLE(0, 0, 0, 0)));
	m.m[1] = _mm_mul_ps(m.m[1], _mm_shuffle_ps(value, value, _MM_SHUFFLE(1, 1, 1, 1)));
	m.m[2] = _mm_mul_ps(m.m[2], _mm_shuffle_ps(value, value, _MM_SHUFFLE(2, 2, 2, 2)));
#else
	m.m[0][0] *= value.m128_f32[0];
	m.m[0][1] *= value.m128_f32[0];
	m.m[0][2] *= value.m128_f32[0];
	m.m[1][0] *= value.m128_f32[1];
	m.m[1][1] *= value.m128_f32[1];
	m.m[1][2] *= value.m128_f32[1];
	m.m[2][0] *= value.m128_f32[2];
	m.m[2][1] *= value.m128_f32[2];
	m.m[2][2] *= value.m128_f32[2];
#endif
}

// apply translation
void StackTranslate(const __m128 value)
{
	//glTranslatef(value.m128_f32[0], value.m128_f32[1], value.m128_f32[2]);
	Matrix4 &m = sStackMatrix[sStackTop];
#ifdef MATRIX_STACK_SSE
	m.m[3] = 
		_mm_add_ps(
			_mm_add_ps(
				_mm_add_ps(
					_mm_mul_ps(m.m[0], _mm_shuffle_ps(value, value, _MM_SHUFFLE(0, 0, 0, 0))),
					_mm_mul_ps(m.m[1], _mm_shuffle_ps(value, value, _MM_SHUFFLE(1, 1, 1, 1)))
				),
				_mm_mul_ps(m.m[2], _mm_shuffle_ps(value, value, _MM_SHUFFLE(2, 2, 2, 2)))
			),
			m.m[3]
		);


#else
	m.m[3][0] += m.m[0][0] * value.m128_f32[0] + m.m[1][0] * value.m128_f32[1] + m.m[2][0] * value.m128_f32[2];
	m.m[3][1] += m.m[0][1] * value.m128_f32[0] + m.m[1][1] * value.m128_f32[1] + m.m[2][1] * value.m128_f32[2];
	m.m[3][2] += m.m[0][2] * value.m128_f32[0] + m.m[1][2] * value.m128_f32[1] + m.m[2][2] * value.m128_f32[2];
#endif
}

// transform a position
__m128 StackTransformPosition(const __m128 value)
{
	const Matrix4 &m = sStackMatrix[sStackTop];
	__m128 p;
#ifdef MATRIX_STACK_SSE
	p = _mm_add_ps(
		_mm_add_ps(
			_mm_add_ps(
				_mm_mul_ps(_mm_shuffle_ps(value, value, _MM_SHUFFLE(0, 0, 0, 0)), m.m[0]),
				_mm_mul_ps(_mm_shuffle_ps(value, value, _MM_SHUFFLE(1, 1, 1, 1)), m.m[1])
			),
			_mm_mul_ps(_mm_shuffle_ps(value, value, _MM_SHUFFLE(2, 2, 2, 2)), m.m[2])
		),
		m.m[3]
	);
#else
	p.m128_f32[0] = value.m128_f32[0] * m.m[0][0] + value.m128_f32[1] * m.m[1][0] + value.m128_f32[2] * m.m[2][0] + m.m[3][0];
	p.m128_f32[1] = value.m128_f32[0] * m.m[0][1] + value.m128_f32[1] * m.m[1][1] + value.m128_f32[2] * m.m[2][1] + m.m[3][1];
	p.m128_f32[2] = value.m128_f32[0] * m.m[0][2] + value.m128_f32[1] * m.m[1][2] + value.m128_f32[2] * m.m[2][2] + m.m[3][2];
	p.m128_f32[3] = 1;
#endif
	return p;
}

// transform a normal
__m128 StackTransformNormal(const __m128 value)
{
	const Matrix4 &m = sStackMatrix[sStackTop];
	__m128 n;
#ifdef MATRIX_STACK_SSE
	n = _mm_add_ps(
		_mm_add_ps(
			_mm_mul_ps(_mm_shuffle_ps(value, value, _MM_SHUFFLE(0, 0, 0, 0)), m.m[0]),
			_mm_mul_ps(_mm_shuffle_ps(value, value, _MM_SHUFFLE(1, 1, 1, 1)), m.m[1])
		),
		_mm_mul_ps(_mm_shuffle_ps(value, value, _MM_SHUFFLE(2, 2, 2, 2)), m.m[2])
	);
#else
	n.m128_f32[0] = value.m128_f32[0] * m.m[0][0] + value.m128_f32[1] * m.m[1][0] + value.m128_f32[2] * m.m[2][0];
	n.m128_f32[1] = value.m128_f32[0] * m.m[0][1] + value.m128_f32[1] * m.m[1][1] + value.m128_f32[2] * m.m[2][1];
	n.m128_f32[2] = value.m128_f32[0] * m.m[0][2] + value.m128_f32[1] * m.m[1][2] + value.m128_f32[2] * m.m[2][2];
	n.m128_f32[3] = 0;
#endif
	float scale = InvSqrt(n.m128_f32[0]*n.m128_f32[0]+n.m128_f32[1]*n.m128_f32[1]+n.m128_f32[2]*n.m128_f32[2]);
	n.m128_f32[0] *= scale;
	n.m128_f32[1] *= scale;
	n.m128_f32[2] *= scale;
	return n;
}

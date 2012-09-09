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
static const int sProjEntries = 4;
static Matrix4 sProjMatrix[sProjEntries];
static int sProjTop;

// view matrix
static Matrix4 sViewMatrix;

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

void ProjectionPush(void)
{
	if (sProjTop < sProjEntries - 1)
	{
		++sProjTop;
		sProjMatrix[sProjTop] = sProjMatrix[sProjTop-1];
	}
	else
	{
		DebugPrint("Projection stack overflow\n");
		assert(false);
	}
}

void ProjectionPop(void)
{
	if (sProjTop > 0)
	{
#ifdef DEBUG
		memset(sProjMatrix + sProjTop, 0, sizeof(Matrix4));
#endif
		--sProjTop;
	}
	else
	{
		DebugPrint("Projection stack underflow\n");
		assert(false);
	}
}

void ProjectionOrtho(float aLeft, float aRight, float aBottom, float aTop, float aNear, float aFar)
{
	const float ortho[16] = 
	{
		2/(aRight-aLeft), 0, 0, 0,
		0, 2/(aTop-aBottom), 0, 0,
		0, 0, -2/(aFar-aNear), 0,
		-(aRight+aLeft)/(aRight-aLeft),-(aTop+aBottom)/(aTop-aBottom),-(aFar+aNear)/(aFar-aNear), 1
	};
	memcpy(&sProjMatrix[sProjTop], ortho, sizeof(Matrix4));
}

void ProjectionFrustum(float aLeft, float aRight, float aBottom, float aTop, float aNear, float aFar)
{
	const float frustum[16] =
	{
		2*aNear/(aRight-aLeft), 0, 0, 0,
		0, 2*aNear/(aTop-aBottom), 0, 0,
		(aRight+aLeft)/(aRight-aLeft), (aTop+aBottom)/(aTop-aBottom), -(aFar+aNear)/(aFar-aNear), -1,
		0, 0, -2*aFar*aNear/(aFar-aNear), 0
	};
	memcpy(&sProjMatrix[sProjTop], frustum, sizeof(Matrix4));
}

void ProjectionLoad(const float *aValues)
{
	memcpy(&sProjMatrix, aValues, sizeof(Matrix4));
}

const float *ProjectionGet(void)
{
	return sProjMatrix[sProjTop].m->m128_f32;
}


//
// VIEW MATRIX OPERATIONS
//

void ViewLoad(const float *aValues)
{
	memcpy(&sViewMatrix, aValues, sizeof(Matrix4));
}

const float *ViewGet(void)
{
	return sViewMatrix.m->m128_f32;
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
	if (sStackTop < sStackEntries - 1)
	{
		++sStackTop;
		sStackMatrix[sStackTop] = sStackMatrix[sStackTop-1];
	}
	else
	{
		DebugPrint("ModelView stack overflow\n");
		assert(false);
	}
}

// pop the current entry
void StackPop(void)
{
	if (sStackTop > 0)
	{
#ifdef DEBUG
		memset(sStackMatrix + sStackTop, 0, sizeof(Matrix4));
#endif
		--sStackTop;
	}
	else
	{
		DebugPrint("ModelView stack underflow\n");
		assert(false);
	}
}

// load an identity matrix
void StackIdentity(void)
{
	Matrix4 &m = sStackMatrix[sStackTop];
	m = Matrix4::Identity;
}

// load a matrix
void StackLoad(const float *aValues)
{
	Matrix4 &m = sStackMatrix[sStackTop];
	memcpy(&m, aValues, sizeof(Matrix4));
}

// multiply by a matrix
void StackMult(const float *aValues)
{
	Matrix4 &m = sStackMatrix[sStackTop];
	Matrix4 b;
	memcpy(&b, aValues, sizeof(Matrix4));
	m = m * b;
}

// apply rotation
void StackRotate(const float aValue)
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
	const __m128 s(_mm_set_ps1(sinf(aValue))), c(_mm_set_ps1(cosf(aValue)));
	m.m[0] = _mm_add_ps(_mm_mul_ps(o.m[0], c), _mm_mul_ps(o.m[1], s));
	m.m[1] = _mm_sub_ps(_mm_mul_ps(o.m[1], c), _mm_mul_ps(o.m[0], s));
	m.m[2] = o.m[2];
	m.m[3] = o.m[3];
#else
	const float s(sinf(aValue*float(M_PI/180.0f))), c(cosf(aValue*float(M_PI/180.0f)));
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
void StackScale(const __m128 aValue)
{
	//glScalef(aValue.m128_f32[0], aValue.m128_f32[1], aValue.m128_f32[2]);
	Matrix4 &m = sStackMatrix[sStackTop];
#ifdef MATRIX_STACK_SSE
	m.m[0] = _mm_mul_ps(m.m[0], _mm_shuffle_ps(aValue, aValue, _MM_SHUFFLE(0, 0, 0, 0)));
	m.m[1] = _mm_mul_ps(m.m[1], _mm_shuffle_ps(aValue, aValue, _MM_SHUFFLE(1, 1, 1, 1)));
	m.m[2] = _mm_mul_ps(m.m[2], _mm_shuffle_ps(aValue, aValue, _MM_SHUFFLE(2, 2, 2, 2)));
#else
	m.m[0][0] *= aValue.m128_f32[0];
	m.m[0][1] *= aValue.m128_f32[0];
	m.m[0][2] *= aValue.m128_f32[0];
	m.m[1][0] *= aValue.m128_f32[1];
	m.m[1][1] *= aValue.m128_f32[1];
	m.m[1][2] *= aValue.m128_f32[1];
	m.m[2][0] *= aValue.m128_f32[2];
	m.m[2][1] *= aValue.m128_f32[2];
	m.m[2][2] *= aValue.m128_f32[2];
#endif
}

// apply translation
void StackTranslate(const __m128 aValue)
{
	//glTranslatef(aValue.m128_f32[0], aValue.m128_f32[1], aValue.m128_f32[2]);
	Matrix4 &m = sStackMatrix[sStackTop];
#ifdef MATRIX_STACK_SSE
	m.m[3] = 
		_mm_add_ps(
			_mm_add_ps(
				_mm_add_ps(
					_mm_mul_ps(m.m[0], _mm_shuffle_ps(aValue, aValue, _MM_SHUFFLE(0, 0, 0, 0))),
					_mm_mul_ps(m.m[1], _mm_shuffle_ps(aValue, aValue, _MM_SHUFFLE(1, 1, 1, 1)))
				),
				_mm_mul_ps(m.m[2], _mm_shuffle_ps(aValue, aValue, _MM_SHUFFLE(2, 2, 2, 2)))
			),
			m.m[3]
		);


#else
	m.m[3][0] += m.m[0][0] * aValue.m128_f32[0] + m.m[1][0] * aValue.m128_f32[1] + m.m[2][0] * aValue.m128_f32[2];
	m.m[3][1] += m.m[0][1] * aValue.m128_f32[0] + m.m[1][1] * aValue.m128_f32[1] + m.m[2][1] * aValue.m128_f32[2];
	m.m[3][2] += m.m[0][2] * aValue.m128_f32[0] + m.m[1][2] * aValue.m128_f32[1] + m.m[2][2] * aValue.m128_f32[2];
#endif
}

// transform a position
__m128 StackTransformPosition(const __m128 aValue)
{
	const Matrix4 &m = sStackMatrix[sStackTop];
	__m128 p;
#ifdef MATRIX_STACK_SSE
	p = _mm_add_ps(
		_mm_add_ps(
			_mm_add_ps(
				_mm_mul_ps(_mm_shuffle_ps(aValue, aValue, _MM_SHUFFLE(0, 0, 0, 0)), m.m[0]),
				_mm_mul_ps(_mm_shuffle_ps(aValue, aValue, _MM_SHUFFLE(1, 1, 1, 1)), m.m[1])
			),
			_mm_mul_ps(_mm_shuffle_ps(aValue, aValue, _MM_SHUFFLE(2, 2, 2, 2)), m.m[2])
		),
		m.m[3]
	);
#else
	p.m128_f32[0] = aValue.m128_f32[0] * m.m[0][0] + aValue.m128_f32[1] * m.m[1][0] + aValue.m128_f32[2] * m.m[2][0] + m.m[3][0];
	p.m128_f32[1] = aValue.m128_f32[0] * m.m[0][1] + aValue.m128_f32[1] * m.m[1][1] + aValue.m128_f32[2] * m.m[2][1] + m.m[3][1];
	p.m128_f32[2] = aValue.m128_f32[0] * m.m[0][2] + aValue.m128_f32[1] * m.m[1][2] + aValue.m128_f32[2] * m.m[2][2] + m.m[3][2];
	p.m128_f32[3] = 1;
#endif
	return p;
}

// transform a normal
__m128 StackTransformNormal(const __m128 aValue)
{
	const Matrix4 &m = sStackMatrix[sStackTop];
	__m128 n;
#ifdef MATRIX_STACK_SSE
	n = _mm_add_ps(
		_mm_add_ps(
			_mm_mul_ps(_mm_shuffle_ps(aValue, aValue, _MM_SHUFFLE(0, 0, 0, 0)), m.m[0]),
			_mm_mul_ps(_mm_shuffle_ps(aValue, aValue, _MM_SHUFFLE(1, 1, 1, 1)), m.m[1])
		),
		_mm_mul_ps(_mm_shuffle_ps(aValue, aValue, _MM_SHUFFLE(2, 2, 2, 2)), m.m[2])
	);
#else
	n.m128_f32[0] = aValue.m128_f32[0] * m.m[0][0] + aValue.m128_f32[1] * m.m[1][0] + aValue.m128_f32[2] * m.m[2][0];
	n.m128_f32[1] = aValue.m128_f32[0] * m.m[0][1] + aValue.m128_f32[1] * m.m[1][1] + aValue.m128_f32[2] * m.m[2][1];
	n.m128_f32[2] = aValue.m128_f32[0] * m.m[0][2] + aValue.m128_f32[1] * m.m[1][2] + aValue.m128_f32[2] * m.m[2][2];
	n.m128_f32[3] = 0;
#endif
	float scale = InvSqrt(n.m128_f32[0]*n.m128_f32[0]+n.m128_f32[1]*n.m128_f32[1]+n.m128_f32[2]*n.m128_f32[2]);
	n.m128_f32[0] *= scale;
	n.m128_f32[1] *= scale;
	n.m128_f32[2] *= scale;
	return n;
}

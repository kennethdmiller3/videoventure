#include "StdAfx.h"

#include "ExpressionSchema.h"

namespace Expression
{
	const char * const Schema<bool>::NAME = "bool";
	const char * const Schema<float>::NAME = "float";
	const char * const Schema<float const>::NAME = "float const";
	const char * const Schema<Vector2>::NAME = "Vector2";
	const char * const Schema<Vector2 const>::NAME = "Vector2 const";
	const char * const Schema<Vector3>::NAME = "Vector3";
	const char * const Schema<Vector3 const>::NAME = "Vector3 const";
	const char * const Schema<Vector4>::NAME = "Vector4";
	const char * const Schema<Vector4 const>::NAME = "Vector4 const";
	const char * const Schema<Color4>::NAME = "Color4";
	const char * const Schema<Color4 const>::NAME = "Color4 const";
	const char * const Schema<__m128>::NAME = "__m128";
	const char * const Schema<__m128 const>::NAME = "__m128 const";
}
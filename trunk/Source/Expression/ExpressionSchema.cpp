#include "StdAfx.h"

#include "ExpressionSchema.h"

namespace Expression
{
	const char * const Schema<bool>::NAME = "bool";
	const char * const Schema<float>::NAME = "float";
	const char * const Schema<float const>::NAME = "float const";
	const char * const Schema<__m128>::NAME = "__m128";
	const char * const Schema<__m128 const>::NAME = "__m128 const";
}
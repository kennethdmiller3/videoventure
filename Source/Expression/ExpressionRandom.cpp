#include "StdAfx.h"

#include "ExpressionRandom.h"
#include "ExpressionConfigure.h"

static ExpressionConfigure::Auto<float> randomfloat(0xa19b8cd6 /* "rand" */, ConfigureRandom<float>);
static ExpressionConfigure::Auto<__m128> randomvector(0xa19b8cd6 /* "rand" */, ConfigureRandom<__m128>);

#include "StdAfx.h"

#include "ExpressionVariable.h"
#include "ExpressionConfigure.h"

static Expression::Loader<float>::Auto variablefloat(0x19385305 /* "variable" */, ConfigureVariable<float>);
static Expression::Loader<__m128>::Auto variablevector(0x19385305 /* "variable" */, ConfigureVariable<__m128>);

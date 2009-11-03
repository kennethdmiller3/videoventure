#include "StdAfx.h"

#include "ExpressionVariable.h"
#include "ExpressionConfigure.h"

static ExpressionConfigure::Auto<float> variablefloat(0x19385305 /* "variable" */, ConfigureVariable<float>);
static ExpressionConfigure::Auto<__m128> variablevector(0x19385305 /* "variable" */, ConfigureVariable<__m128>);

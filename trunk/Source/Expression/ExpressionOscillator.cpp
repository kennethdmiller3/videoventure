#include "StdAfx.h"

#include "Expression.h"
#include "ExpressionOperator.h"
#include "ExpressionConfigure.h"
#include "ExpressionIntegral.h"
#include "ExpressionConvert.h"
#include "ExpressionOscillator.h"

//
template<typename T> static void ConfigureSineWave(const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[])
{
	Expression::Convert<T, float>::Append(buffer);
	ConfigureSineWave(element, buffer);
}

static ExpressionConfigure::Auto<float> sinewavefloat(0xb711f539 /* "sinewave" */, ConfigureSineWave<float>);
static ExpressionConfigure::Auto<__m128> sinewavevector(0xb711f539 /* "sinewave" */, ConfigureSineWave<__m128>);

//
template<typename T> static void ConfigureTriangleWave(const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[])
{
	Expression::Convert<T, float>::Append(buffer);
	ConfigureTriangleWave(element, buffer);
}

static ExpressionConfigure::Auto<float> trianglewavefloat(0xd0308494 /* "trianglewave" */, ConfigureTriangleWave<float>);
static ExpressionConfigure::Auto<__m128> trianglewavevector(0xd0308494 /* "trianglewave" */, ConfigureTriangleWave<__m128>);

//
template<typename T> static void ConfigureSawtoothWave(const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[])
{
	Expression::Convert<T, float>::Append(buffer);
	ConfigureSawtoothWave(element, buffer);
}

static ExpressionConfigure::Auto<float> sawtoothwavefloat(0x705614d5 /* "sawtoothwave" */, ConfigureSawtoothWave<float>);
static ExpressionConfigure::Auto<__m128> sawtoothwavevector(0x705614d5 /* "sawtoothwave" */, ConfigureSawtoothWave<__m128>);


//
template<typename T> static void ConfigurePulseWave(const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[])
{
	Expression::Convert<T, float>::Append(buffer);
	ConfigurePulseWave(element, buffer);
}

static ExpressionConfigure::Auto<float> pulsewavefloat(0x3f8dc467 /* "pulsewave" */, ConfigurePulseWave<float>);
static ExpressionConfigure::Auto<__m128> pulsewavevector(0x3f8dc467 /* "pulsewave" */, ConfigurePulseWave<__m128>);


//
// OSCILLATOR EXPRESSION
//

// configue a parameter
static bool ConfigureParameter(const TiXmlElement *element, const char *param, std::vector<unsigned int> &buffer, float (*op)(Expression::Context &), const char * const names[], const float defvalue, const float identity)
{
	// get constant value from attribute
	float value = defvalue;
	if (element->QueryFloatAttribute(param, &value) == TIXML_WRONG_TYPE)
	{
		// not a number; try as a name
		if (const char *name = element->Attribute(param))
		{
			// append the operation (if any)
			if (op)
				Expression::Append(buffer, op);

			// attribute variable reference
			Expression::Append(buffer, EvaluateVariable<float>, Hash(name));
			return true;
		}
	}

	// if there is a child tag for the parameter...
	if (const TiXmlElement *child = element->FirstChildElement(param))
	{
		// append the operation (if any)
		if (op)
			Expression::Append(buffer, op);

		// configure the expression
		ConfigureExpressionRoot<float>(child, buffer, names, &value);
		return true;
	}

	// if the value is not identity...
	if (value != identity)
	{
		// append the operation (if any)
		if (op)
			Expression::Append(buffer, op);

		// append a constant expression
		Expression::Append(buffer, Expression::Constant<float>, value);
		return true;
	}

	// no parameter found
	return false;
}


//
// SINE WAVE OSCILLATOR
//

// primitive sine wave oscillator
static float SineWave(Expression::Context &aContext)
{
	float arg1(Expression::Evaluate<float>(aContext));
	return sinf(2.0f * float(M_PI) * arg1);
}

// configure sine wave oscillator
void ConfigureSineWave(const TiXmlElement *element, std::vector<unsigned int> &buffer)
{
	// configure offset
	ConfigureParameter(element, "offset", buffer, Expression::Add<float>, sScalarNames, 0.0f, 0.0f);

	// configure scale
	ConfigureParameter(element, "scale", buffer, Expression::Mul<float>, sScalarNames, 1.0f, 1.0f);

	// sine wave
	Expression::Append(buffer, SineWave);

	// configure phase
	ConfigureParameter(element, "phase", buffer, Expression::Add<float>, sScalarNames, 0.0f, 0.0f);

	// append an integrator
	Expression::Append(buffer, EvaluateIntegral);

	// get input
	if (const TiXmlElement *child = element->FirstChildElement("input"))
	{
		// get input expression
		ConfigureExpressionRoot<float>(child, buffer, sScalarNames, sScalarDefault);
	}
	else if (const char *input = element->Attribute("input"))
	{
		// attribute variable reference
		Expression::Append(buffer, EvaluateVariable<float>, Hash(input));
	}
	else
	{
		// default to time
		Expression::Append(buffer, EvaluateTime);
	}

	// frequency
	ConfigureParameter(element, "period", buffer, Expression::Rcp<float>, sScalarNames, 1.0f, 1.0f) ||
	ConfigureParameter(element, "frequency", buffer, NULL, sScalarNames, 1.0f, FLT_MAX);
}


//
// TRIANGLE WAVE OSCILLATOR
//

// primitive triangle wave oscillator
static float TriangleWave(Expression::Context &aContext)
{
	float arg1(Expression::Evaluate<float>(aContext) + 0.25f);
	return 2.0f * fabsf(2.0f * (arg1 - xs_RoundToInt(arg1))) - 1.0f;
}

// configure triangle wave oscillator
void ConfigureTriangleWave(const TiXmlElement *element, std::vector<unsigned int> &buffer)
{
	// configure offset
	ConfigureParameter(element, "offset", buffer, Expression::Add<float>, sScalarNames, 0.0f, 0.0f);

	// configure scale
	ConfigureParameter(element, "scale", buffer, Expression::Mul<float>, sScalarNames, 1.0f, 1.0f);

	// triangle wave
	Expression::Append(buffer, TriangleWave);

	// configure phase
	ConfigureParameter(element, "phase", buffer, Expression::Add<float>, sScalarNames, 0.0f, 0.0f);

	// append an integrator
	Expression::Append(buffer, EvaluateIntegral);

	// get input
	if (const TiXmlElement *child = element->FirstChildElement("input"))
	{
		// get input expression
		ConfigureExpressionRoot<float>(child, buffer, sScalarNames, sScalarDefault);
	}
	else
	{
		// default to time
		Expression::Append(buffer, EvaluateTime);
	}

	// frequency
	ConfigureParameter(element, "period", buffer, Expression::Rcp<float>, sScalarNames, 1.0f, 1.0f) ||
	ConfigureParameter(element, "frequency", buffer, NULL, sScalarNames, 1.0f, FLT_MAX);
}


//
// SAWTOOTH WAVE OSCILLATOR
//

// primitive sawtooth wave oscillator
static float SawtoothWave(Expression::Context &aContext)
{
	float arg1(Expression::Evaluate<float>(aContext));
	return 2.0f * (arg1 - xs_RoundToInt(arg1));
}

// configure sawtooth wave oscillator
void ConfigureSawtoothWave(const TiXmlElement *element, std::vector<unsigned int> &buffer)
{
	// configure offset
	ConfigureParameter(element, "offset", buffer, Expression::Add<float>, sScalarNames, 0.0f, 0.0f);

	// configure scale
	ConfigureParameter(element, "scale", buffer, Expression::Mul<float>, sScalarNames, 1.0f, 1.0f);

	// sawtooth wave
	Expression::Append(buffer, SawtoothWave);

	// configure phase
	ConfigureParameter(element, "phase", buffer, Expression::Add<float>, sScalarNames, 0.0f, 0.0f);

	// append an integrator
	Expression::Append(buffer, EvaluateIntegral);

	// get input
	if (const TiXmlElement *child = element->FirstChildElement("input"))
	{
		// get input expression
		ConfigureExpressionRoot<float>(child, buffer, sScalarNames, sScalarDefault);
	}
	else
	{
		// default to time
		Expression::Append(buffer, EvaluateTime);
	}

	// frequency
	ConfigureParameter(element, "period", buffer, Expression::Rcp<float>, sScalarNames, 1.0f, 1.0f) ||
	ConfigureParameter(element, "frequency", buffer, NULL, sScalarNames, 1.0f, FLT_MAX);
}


//
// PULSE WAVE OSCILLATOR
//

// primitive pulse wave oscillator
static float PulseWave(Expression::Context &aContext)
{
	float arg1(Expression::Evaluate<float>(aContext));
	float arg2(Expression::Evaluate<float>(aContext));
	return (arg1 - xs_FloorToInt(arg1) < arg2 - xs_FloorToInt(arg2)) ? 1.0f : -1.0f;
}

// configure pulse wave oscillator
void ConfigurePulseWave(const TiXmlElement *element, std::vector<unsigned int> &buffer)
{
	// configure offset
	ConfigureParameter(element, "offset", buffer, Expression::Add<float>, sScalarNames, 0.0f, 0.0f);

	// configure scale
	ConfigureParameter(element, "scale", buffer, Expression::Mul<float>, sScalarNames, 1.0f, 1.0f);

	// pulse wave
	Expression::Append(buffer, PulseWave);

	// configure phase
	ConfigureParameter(element, "phase", buffer, Expression::Add<float>, sScalarNames, 0.0f, 0.0f);

	// append an integrator
	Expression::Append(buffer, EvaluateIntegral);

	// get input
	if (const TiXmlElement *child = element->FirstChildElement("input"))
	{
		// get input expression
		ConfigureExpressionRoot<float>(child, buffer, sScalarNames, sScalarDefault);
	}
	else
	{
		// default to time
		Expression::Append(buffer, EvaluateTime);
	}

	// frequency
	ConfigureParameter(element, "period", buffer, Expression::Rcp<float>, sScalarNames, 1.0f, 1.0f) ||
	ConfigureParameter(element, "frequency", buffer, NULL, sScalarNames, 1.0f, FLT_MAX);

	// apply duty cycle
	ConfigureParameter(element, "duty", buffer, NULL, sScalarNames, 0.5f, FLT_MAX);
}

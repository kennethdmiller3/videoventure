#include "StdAfx.h"

#include "ExpressionRandom.h"
#include "ExpressionConfigure.h"

static Expression::Loader<float>::Auto randomfloat(0xa19b8cd6 /* "rand" */, ConfigureRandom<float>);
static Expression::Loader<__m128>::Auto randomvector(0xa19b8cd6 /* "rand" */, ConfigureRandom<__m128>);

// configure typed random
template <typename T> void ConfigureRandom(const tinyxml2::XMLElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[])
{
	// width in floats (HACK)
	const int width = (sizeof(T)+sizeof(float)-1)/sizeof(float);

	// get random count
	int count = 1;
	element->QueryIntAttribute("rand", &count);

#ifdef PRINT_CONFIGURE_EXPRESSION
	DebugPrint("%s random %d:", Expression::Schema<T>::NAME, count);
#endif

	// offset factor
	float offset[width];

	// scale factor
	float scale[width];

	// status flags
	bool need_offset = false;
	bool need_scale = false;
	bool need_rand = false;

	// get component properties
	for (int i = 0; i < width; i++)
	{
		char label[64];

		// default values
		offset[i] = defaults[i];
		scale[i] = 0;

		// average
		sprintf(label, "%s_avg", names[i]);
		float average = defaults[i];
		if (element->QueryFloatAttribute(label, &average) == tinyxml2::XML_SUCCESS)
		{
			offset[i] = average;
		}

		// variance
		sprintf(label, "%s_var", names[i]);
		float variance = 0.0f;
		if (element->QueryFloatAttribute(label, &variance) == tinyxml2::XML_SUCCESS)
		{
			offset[i] = average - variance;
			scale[i] = variance * 2;
		}

		// minimum
		sprintf(label, "%s_min", names[i]);
		float minimum = 0.0f;
		if (element->QueryFloatAttribute(label, &minimum) == tinyxml2::XML_SUCCESS)
		{
			offset[i] = minimum;
		}

		// maximum
		sprintf(label, "%s_max", names[i]);
		float maximum = 0.0f;
		if (element->QueryFloatAttribute(label, &maximum) == tinyxml2::XML_SUCCESS)
		{
			scale[i] = maximum - minimum;
		}

		// update status
		if (offset[i] != 0.0f)
			need_offset = true;

		if (count > 0)
		{
			// compensate for random count
			scale[i] /= count;

			if (scale[i] != 0.0f)
				need_rand = true;
			if (scale[i] != 1.0f)
				need_scale = true;
		}

#ifdef PRINT_CONFIGURE_EXPRESSION
		DebugPrint(" %f+%f*%s", offset[i], scale[i], names[i]);
#endif
	}

	if (need_offset || !need_rand)
	{
		if (need_rand)
		{
			// push add
			Expression::Append(buffer, Expression::Add<T>);
		}

		// push offset
		Expression::Append(buffer, Expression::Constant<T>);
		for (int i = 0; i < width; ++i)
			Expression::New<float>(buffer, offset[i]);
	}

	if (need_rand)
	{
		if (need_scale)
		{
			// push multiply
			Expression::Append(buffer, Expression::Mul<T>);

			// push scale
			Expression::Append(buffer, Expression::Constant<T>);
			for (int i = 0; i < width; ++i)
				Expression::New<float>(buffer, scale[i]);
		}

		for (int i = 0; i < count; ++i)
		{
			if (count > 1 && i < count - 1)
			{
				// push add
				Expression::Append(buffer, Expression::Add<T>);
			}

			// push randoms
			Expression::Append(buffer, Expression::ComponentNullary<T, Expression::Schema<T>::COUNT>::Evaluate<float, Random::Float>);
		}
	}

#ifdef PRINT_CONFIGURE_EXPRESSION
	DebugPrint("\n");
#endif
}

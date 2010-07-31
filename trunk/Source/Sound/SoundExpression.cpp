#include "StdAfx.h"

#include "Sound.h"
#include "SoundConfigure.h"
#include "ExpressionConfigure.h"

static bool Configure(SoundTemplate &self, const TiXmlElement *element, unsigned int id)
{
	// get sound length
	float length;
	element->QueryFloatAttribute("length", &length);
	size_t count = xs_CeilToInt(length * AUDIO_FREQUENCY);

	// reserve space
	self.Reserve(count);

	// get expression
	std::vector<unsigned int> buffer;
	ConfigureExpressionRoot<float>(element, buffer, sScalarNames, sScalarDefault);

	// set up a context
	EntityContext context(&buffer[0], buffer.size(), 0, id);

	// for each sample...
	for (size_t i = 0; i < count; ++i, context.Restart())
	{
		// evaluate the expression
		context.mParam = float(i) / AUDIO_FREQUENCY;
		float value = Expression::Evaluate<float>(context);

		// add a sample
		self.Append(short(Clamp(xs_RoundToInt(value * SHRT_MAX), SHRT_MIN, SHRT_MAX)));
	}

	return true;
}

SoundConfigure::Auto soundexpression(0xcf15afeb /* "expression" */, Configure);
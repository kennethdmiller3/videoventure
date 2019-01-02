#include "StdAfx.h"

#include "Sound.h"
#include "SoundConfigure.h"
#include "ExpressionConfigure.h"

static bool Configure(SoundTemplate &self, const tinyxml2::XMLElement *element, unsigned int id)
{
	// get sound length
	float length;
	element->QueryFloatAttribute("length", &length);
	int count = int(ceilf(length * AUDIO_FREQUENCY));

	// reserve space
	self.Reserve(count);

	// get expression
	std::vector<unsigned int> buffer;
	Expression::Loader<float>::ConfigureRoot(element, buffer, sScalarNames, sScalarDefault);

	// set up a context
	EntityContext context(&buffer[0], buffer.size(), 0, id);

	// for each sample...
	for (int i = 0; i < count; ++i, context.Restart())
	{
		// evaluate the expression
		context.mParam = float(i) / AUDIO_FREQUENCY;
		float value = Expression::Evaluate<float>(context);

		// add a sample
		self.Append(short(Clamp(int(floorf(0.5f + value * SHRT_MAX)), SHRT_MIN, SHRT_MAX)));
	}

	return true;
}

static SoundConfigure::Configure soundexpression(0xcf15afeb /* "expression" */, Configure);
#include "StdAfx.h"
#include "Cloud.h"
#include "Drawlist.h"


namespace Database
{
	namespace Loader
	{
		class CloudLoader
		{
		public:
			CloudLoader()
			{
				AddConfigure(0x1ac6a97e /* "cloud" */, Entry(this, &CloudLoader::Configure));
			}

			void Configure(unsigned int aId, const TiXmlElement *element)
			{
				ProcessCloudItems(element);
			}
		}
		cloudloader;
	}

}

inline float rand_float()
{
	return (float)rand() * (1.0f / (float)RAND_MAX);
}
inline float rand_value(float aAverage, float aVariance)
{
	return (2.0f * rand_float() - 1.0f) * aVariance + aAverage;
}

GLuint CreateCloudDrawList(int aCount,
						   float aPosAverage[], float aPosVariance[],
						   float aWidthAverage, float aWidthVariance,
						   float aHeightAverage, float aHeightVariance,
						   float aColorAverage[], float aColorVariance[])
{
	// create a new draw list
	GLuint handle = glGenLists(1);
	glNewList(handle, GL_COMPILE);

	// begin primitive
	glBegin( GL_QUADS );

	// for each cloud...
	for (int i = 0; i < aCount; i++)
	{
		// randomize position
		float x = rand_value(aPosAverage[0], aPosVariance[0]);
		float y = rand_value(aPosAverage[1], aPosVariance[1]);
#ifdef DRAW_FRONT_TO_BACK
		float z = aPosAverage[2] + (2 * i - aCount) * aPosVariance[2] / aCount;
#else
		float z = aPosAverage[2] + (aCount - 2 * i) * aPosVariance[2] / aCount;
#endif

		// randomize size
		float w = rand_value(aWidthAverage, aWidthVariance);
		float h = rand_value(aHeightAverage, aHeightVariance);

		// randomize color
		glColor4f(
			rand_value(aColorAverage[0], aColorVariance[0]),
			rand_value(aColorAverage[1], aColorVariance[1]),
			rand_value(aColorAverage[2], aColorVariance[2]),
			rand_value(aColorAverage[3], aColorVariance[3])
			);

		// submit vertices
		glVertex3f( x - w, y - h, z );
		glVertex3f( x + w, y - h, z );
		glVertex3f( x + w, y + h, z );
		glVertex3f( x - w, y + h, z );
	}

	// end primitive
	glEnd();

	// finish the draw list
	glEndList();

	// return the draw list
	return handle;
}

void ProcessCloudItem(const TiXmlElement *element, float &average, float &variance)
{
	element->QueryFloatAttribute("average", &average);
	element->QueryFloatAttribute("variance", &variance);
}

GLuint ProcessCloudItems(const TiXmlElement *element)
{
	// cloud count
	int count = 1;
	element->QueryIntAttribute("count", &count);

	// default values
	float pos_average[3] = { 0, 0, 640 }, pos_variance[3] = { 2048, 2048, 384 };
	float width_average = 256, width_variance = 192;
	float height_average = 256, height_variance = 192;
	float color_average[4] = { 0.375f, 0.375f, 0.375f, 0.75f }, color_variance[4] = { 0.125f, 0.125f, 0.125f, 0.0f };

	// read values
	for (const TiXmlElement *child = element->FirstChildElement(); child; child = child->NextSiblingElement())
	{
		switch (Hash(child->Value()))
		{
		case 0xfd0c5087 /* "x" */:		ProcessCloudItem(child, pos_average[0], pos_variance[0]); break;
		case 0xfc0c4ef4 /* "y" */:		ProcessCloudItem(child, pos_average[1], pos_variance[1]); break;
		case 0xff0c53ad /* "z" */:		ProcessCloudItem(child, pos_average[2], pos_variance[2]); break;
		case 0x95876e1f /* "width" */:	ProcessCloudItem(child, width_average, width_variance); break;
		case 0xd5bdbb42 /* "height" */:	ProcessCloudItem(child, height_average, height_variance); break;
		case 0x40f480dc /* "red" */:	ProcessCloudItem(child, color_average[0], color_variance[0]); break;
		case 0x011decbc /* "green" */:	ProcessCloudItem(child, color_average[1], color_variance[1]); break;
		case 0x82fbf5cd /* "blue" */:	ProcessCloudItem(child, color_average[2], color_variance[2]); break;
		case 0x5d8b6dab /* "alpha" */:	ProcessCloudItem(child, color_average[3], color_variance[3]); break;
		}
	}

	// create the draw list
	GLuint handle = CreateCloudDrawList(count,
		pos_average, pos_variance,
		width_average, width_variance,
		height_average, height_variance,
		color_average, color_variance
		);

	// get the list name
	const char *name = element->Attribute("name");
	if (name)
	{
		// register the draw list
		Database::drawlist.Put(Hash(name), handle);
	}

	//
	return handle;
}

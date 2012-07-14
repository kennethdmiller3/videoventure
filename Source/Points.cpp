#include "StdAfx.h"
#include "Points.h"

namespace Database
{
	// point value database
	Typed<int> points(0xbc9567c6 /* "points" */);

	namespace Loader
	{
		static void PointsConfigure(unsigned int aId, const tinyxml2::XMLElement *element)
		{
			int &points = Database::points.Open(aId);
			element->QueryIntAttribute("value", &points);
			Database::points.Close(aId);
		}
		Configure pointsconfigure(0xbc9567c6 /* "points" */, PointsConfigure);
	}
}

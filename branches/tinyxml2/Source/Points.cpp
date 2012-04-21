#include "StdAfx.h"
#include "Points.h"

namespace Database
{
	// point value database
	Typed<int> points(0xbc9567c6 /* "points" */);

	namespace Loader
	{
		class PointsLoader
		{
		public:
			PointsLoader()
			{
				AddConfigure(0xbc9567c6 /* "points" */, Entry(this, &PointsLoader::Configure));
			}

			void Configure(unsigned int aId, const tinyxml2::XMLElement *element)
			{
				int &points = Database::points.Open(aId);
				element->QueryIntAttribute("value", &points);
				Database::points.Close(aId);
			}
		}
		pointsloader;
	}
}

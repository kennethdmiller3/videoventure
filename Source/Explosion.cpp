#include "StdAfx.h"
#include "Explosion.h"
#include <boost/pool/pool.hpp>

namespace Database
{
	Typed<ExplosionTemplate> explosiontemplate("explosiontemplate");
	Typed<Explosion *> explosion("explosion");
};


// explosion pool
static boost::pool<boost::default_user_allocator_malloc_free> pool(sizeof(Explosion));
void *Explosion::operator new(size_t aSize)
{
	return pool.malloc();
}
void Explosion::operator delete(void *aPtr)
{
	pool.free(aPtr);
}


ExplosionTemplate::ExplosionTemplate(void)
: mLifeSpan( 0.25f )
{
}

ExplosionTemplate::~ExplosionTemplate(void)
{
}

bool ExplosionTemplate::ConfigureScale(TiXmlElement *element, ExplosionTemplate::ScaleKeys &scalekeys)
{
	// process child elements
	for (TiXmlElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
	{
		const char *label = child->Value();
		switch (Hash(label))
		{
		case 0x82971c71 /* "scale" */:
			{
				float time = 0.0f;
				child->QueryFloatAttribute("time", &time);
				Scale &scale = scalekeys[time];
				child->QueryFloatAttribute("x", &scale.x);
				child->QueryFloatAttribute("y", &scale.y);
				child->QueryFloatAttribute("z", &scale.z);
			}
			break;
		}
	}
	return true;
}

bool ExplosionTemplate::ConfigureColor(TiXmlElement *element, ExplosionTemplate::ColorKeys &colorkeys)
{
	// process child elements
	for (TiXmlElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
	{
		const char *label = child->Value();
		switch (Hash(label))
		{
		case 0x3d7e6258 /* "color" */:
			{
				float time = 0.0f;
				child->QueryFloatAttribute("time", &time);
				ExplosionTemplate::Color &color = colorkeys[time];
				child->QueryFloatAttribute("r", &color.r);
				child->QueryFloatAttribute("g", &color.g);
				child->QueryFloatAttribute("b", &color.b);
				child->QueryFloatAttribute("a", &color.a);
			}
			break;
		}
	}
	return true;
}

bool ExplosionTemplate::Configure(TiXmlElement *element)
{
	if (Hash(element->Value()) != 0x02bb1fe0 /* "explosion" */)
		return false;

	element->QueryFloatAttribute("life", &mLifeSpan);

	int coreindex = 0;
	int haloindex = 0;

	// process child elements
	for (TiXmlElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
	{
		const char *label = child->Value();
		switch (Hash(label))
		{
		case 0xc5537ae1 /* "corecolor" */:
			ConfigureColor(child, mCoreColor);
			break;

		case 0x64a2ab1c /* "corescale" */:
			ConfigureScale(child, mCoreScale);
			break;

		case 0xe0cab0ae /* "halocolor" */:
			ConfigureColor(child, mHaloColor);
			break;

		case 0xcec79d1f /* "haloscale" */:
			ConfigureScale(child, mHaloScale);
			break;
		}
	}

	return true;
}


Explosion::Explosion(void)
: ExplosionTemplate()
, Simulatable(0)
, Renderable()
, mLife(0)
{
}

Explosion::Explosion(const ExplosionTemplate &aTemplate, unsigned int aId)
: ExplosionTemplate(aTemplate)
, Simulatable(aId)
, Renderable(RenderableTemplate(), aId)
, mLife(aTemplate.mLifeSpan)
{
	// get a circle drawlist
	mDraw = Database::drawlist.Get(0x8cdedbba /* "circle16" */);

	// add as a renderable (HACK)
	Database::renderable.Put(Simulatable::id, this);

	// set as visible
	Renderable::Show();
}

Explosion::~Explosion(void)
{
	// remove as a renderable (HACK)
	Database::renderable.Delete(Simulatable::id);
}

void Explosion::Simulate(float aStep)
{
	// advance life timer
	mLife -= aStep;
	if (mLife <= 0)
	{
		Database::Delete(Simulatable::id);
		return;
	}
}

void Explosion::Render(const Matrix2 &transform)
{
	// elapsed time
	float t = mLifeSpan - mLife;

	// draw explosion core
	glPushMatrix();
	const ColorKeys::const_iterator corecolorkey0 = --mCoreColor.upper_bound(t);
	const ColorKeys::const_iterator corecolorkey1 = mCoreColor.upper_bound(t);
	const Color &corecolor0 = corecolorkey0->second;
	const Color &corecolor1 = corecolorkey1->second;
	float corecolorf = (t - corecolorkey0->first) / (corecolorkey1->first - corecolorkey0->first + 0.001f);
	glColor4f(
		corecolor0.r + (corecolor1.r - corecolor0.r) * corecolorf,
		corecolor0.g + (corecolor1.g - corecolor0.g) * corecolorf,
		corecolor0.b + (corecolor1.b - corecolor0.b) * corecolorf,
		corecolor0.a + (corecolor1.a - corecolor0.a) * corecolorf
	);
	const ScaleKeys::const_iterator corescalekey0 = --mCoreScale.upper_bound(t);
	const ScaleKeys::const_iterator corescalekey1 = mCoreScale.upper_bound(t);
	const Scale &corescale0 = corescalekey0->second;
	const Scale &corescale1 = corescalekey1->second;
	float corescalef = (t - corescalekey0->first) / (corescalekey1->first - corescalekey0->first + 0.001f);
	glScalef(
		corescale0.x + (corescale1.x - corescale0.x) * corescalef,
		corescale0.y + (corescale1.y - corescale0.y) * corescalef,
		corescale0.z + (corescale1.z - corescale0.z) * corescalef
		);
	glCallList(mDraw);
	glPopMatrix();

	// draw explosion halo
	glPushMatrix();
	const ColorKeys::const_iterator halocolorkey0 = --mHaloColor.upper_bound(t);
	const ColorKeys::const_iterator halocolorkey1 = mHaloColor.upper_bound(t);
	const Color &halocolor0 = halocolorkey0->second;
	const Color &halocolor1 = halocolorkey1->second;
	float halocolorf = (t - halocolorkey0->first) / (halocolorkey1->first - halocolorkey0->first + 0.001f);
	glColor4f(
		halocolor0.r + (halocolor1.r - halocolor0.r) * halocolorf,
		halocolor0.g + (halocolor1.g - halocolor0.g) * halocolorf,
		halocolor0.b + (halocolor1.b - halocolor0.b) * halocolorf,
		halocolor0.a + (halocolor1.a - halocolor0.a) * halocolorf
	);
	const ScaleKeys::const_iterator haloscalekey0 = --mHaloScale.upper_bound(t);
	const ScaleKeys::const_iterator haloscalekey1 = mHaloScale.upper_bound(t);
	const Scale &haloscale0 = haloscalekey0->second;
	const Scale &haloscale1 = haloscalekey1->second;
	float haloscalef = (t - haloscalekey0->first) / (haloscalekey1->first - haloscalekey0->first + 0.001f);
	glScalef(
		haloscale0.x + (haloscale1.x - haloscale0.x) * haloscalef,
		haloscale0.y + (haloscale1.y - haloscale0.y) * haloscalef,
		haloscale0.z + (haloscale1.z - haloscale0.z) * haloscalef
		);
	glCallList(mDraw);
	glPopMatrix();
}

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

bool ExplosionTemplate::ConfigureScaleLinear(TiXmlElement *element, ExplosionTemplate::ScaleLinear &scale)
{
	// process child elements
	for (TiXmlElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
	{
		const char *label = child->Value();
		switch (Hash(label))
		{
		case 0x652b04df /* "start" */:
			child->QueryFloatAttribute("x", &scale.start.x);
			child->QueryFloatAttribute("y", &scale.start.y);
			child->QueryFloatAttribute("z", &scale.start.z);
			break;

		case 0x6a8e75aa /* "end" */:
			child->QueryFloatAttribute("x", &scale.rate.x);
			child->QueryFloatAttribute("y", &scale.rate.y);
			child->QueryFloatAttribute("z", &scale.rate.z);
			break;
		}
	}
	return true;
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
				scalekeys.push_back(ScaleLinear());
				ScaleLinear &scale = scalekeys.back();
				child->QueryFloatAttribute("time", &scale.time);
				ConfigureScaleLinear(child, scale);
				scale.rate.x = (scale.rate.x - scale.start.x) / scale.time;
				scale.rate.y = (scale.rate.y - scale.start.y) / scale.time;
				scale.rate.z = (scale.rate.z - scale.start.z) / scale.time;
			}
			break;
		}
	}
	return true;
}

bool ExplosionTemplate::ConfigureColorLinear(TiXmlElement *element, ExplosionTemplate::ColorLinear &color)
{
	// process child elements
	for (TiXmlElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
	{
		const char *label = child->Value();
		switch (Hash(label))
		{
		case 0x652b04df /* "start" */:
			child->QueryFloatAttribute("r", &color.start.r);
			child->QueryFloatAttribute("g", &color.start.g);
			child->QueryFloatAttribute("b", &color.start.b);
			child->QueryFloatAttribute("a", &color.start.a);
			break;

		case 0x6a8e75aa /* "end" */:
			child->QueryFloatAttribute("r", &color.rate.r);
			child->QueryFloatAttribute("g", &color.rate.g);
			child->QueryFloatAttribute("b", &color.rate.b);
			child->QueryFloatAttribute("a", &color.rate.a);
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
				colorkeys.push_back(ColorLinear());
				ColorLinear &color = colorkeys.back();
				child->QueryFloatAttribute("time", &color.time);
				ConfigureColorLinear(child, color);
				color.rate.r = (color.rate.r - color.start.r) / color.time;
				color.rate.g = (color.rate.g - color.start.g) / color.time;
				color.rate.b = (color.rate.b - color.start.b) / color.time;
				color.rate.a = (color.rate.a - color.start.a) / color.time;
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

	//
	// DRAW EXPLOSION CORE

	glPushMatrix();

	// interpolate color
	{
		float time = t;
		Color color = Color();
		for (ColorKeys::const_iterator colorkey = mCoreColor.begin(); colorkey != mCoreColor.end(); ++colorkey)
		{
			if (time < colorkey->time)
			{
				color.r = colorkey->start.r + time * colorkey->rate.r;
				color.g = colorkey->start.g + time * colorkey->rate.g;
				color.b = colorkey->start.b + time * colorkey->rate.b;
				color.a = colorkey->start.a + time * colorkey->rate.a;
				break;
			}
			time -= colorkey->time;
		}
		glColor4f(color.r, color.g, color.b, color.a);
	}

	// interpolate scale
	{
		float time = t;
		Scale scale = Scale();
		for (ScaleKeys::const_iterator scalekey = mCoreScale.begin(); scalekey != mCoreScale.end(); ++scalekey)
		{
			if (time < scalekey->time)
			{
				scale.x = scalekey->start.x + time * scalekey->rate.x;
				scale.y = scalekey->start.y + time * scalekey->rate.y;
				scale.z = scalekey->start.z + time * scalekey->rate.z;
				break;
			}
			time -= scalekey->time;
		}
		glScalef(scale.x, scale.y, scale.z );
	}

	// draw core
	glCallList(mDraw);

	glPopMatrix();


	//
	// DRAW EXPLOSION HALO

	glPushMatrix();

	// interpolate color
	{
		float time = t;
		Color color = Color();
		for (ColorKeys::const_iterator colorkey = mHaloColor.begin(); colorkey != mHaloColor.end(); ++colorkey)
		{
			if (time < colorkey->time)
			{
				color.r = colorkey->start.r + time * colorkey->rate.r;
				color.g = colorkey->start.g + time * colorkey->rate.g;
				color.b = colorkey->start.b + time * colorkey->rate.b;
				color.a = colorkey->start.a + time * colorkey->rate.a;
				break;
			}
			time -= colorkey->time;
		}
		glColor4f(color.r, color.g, color.b, color.a);
	}

	// interpolate scale
	{
		float time = t;
		Scale scale = Scale();
		for (ScaleKeys::const_iterator scalekey = mHaloScale.begin(); scalekey != mHaloScale.end(); ++scalekey)
		{
			if (time < scalekey->time)
			{
				scale.x = scalekey->start.x + time * scalekey->rate.x;
				scale.y = scalekey->start.y + time * scalekey->rate.y;
				scale.z = scalekey->start.z + time * scalekey->rate.z;
				break;
			}
			time -= scalekey->time;
		}
		glScalef(scale.x, scale.y, scale.z );
	}

	// draw halo
	glCallList(mDraw);

	glPopMatrix();
}

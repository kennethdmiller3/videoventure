#include "StdAfx.h"

#include "Spawn.h"

SpawnTemplate::SpawnTemplate(void)
: mOffset(0, Vector2(0, 0))
, mScatter(0, Vector2(0, 0))
, mInherit(0, Vector2(1, 1))
, mVelocity(0, Vector2(0, 0))
, mVariance(0, Vector2(0, 0))
, mSpawn(0)
{
}

bool SpawnTemplate::Configure(const TiXmlElement *element, unsigned int aId)
{
	if (const char *spawn = element->Attribute("name"))
		mSpawn = Hash(spawn);
	if (const char *spawn = element->Attribute("spawn"))
		mSpawn = Hash(spawn);

	// process child elements
	for (const TiXmlElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
	{
		unsigned int aPropId = Hash(child->Value());
		switch (aPropId)
		{
		case 0x14c8d3ca /* "offset" */:
			{
				if (child->QueryFloatAttribute("angle", &mOffset.a) == TIXML_SUCCESS)
					mOffset.a *= float(M_PI) / 180.0f;
				child->QueryFloatAttribute("x", &mOffset.p.x);
				child->QueryFloatAttribute("y", &mOffset.p.y);
			}
			break;

		case 0xcab7a341 /* "scatter" */:
			{
				if (child->QueryFloatAttribute("angle", &mScatter.a) == TIXML_SUCCESS)
					mScatter.a *= float(M_PI) / 180.0f;
				child->QueryFloatAttribute("x", &mScatter.p.x);
				child->QueryFloatAttribute("y", &mScatter.p.y);
			}
			break;

		case 0xca04efe0 /* "inherit" */:
			{
				if (child->QueryFloatAttribute("angle", &mInherit.a) == TIXML_SUCCESS)
					mInherit.a *= float(M_PI) / 180.0f;
				child->QueryFloatAttribute("x", &mInherit.p.x);
				child->QueryFloatAttribute("y", &mInherit.p.y);
			}
			break;

		case 0x32741c32 /* "velocity" */:
			{
				if (child->QueryFloatAttribute("angle", &mVelocity.a) == TIXML_SUCCESS)
					mVelocity.a *= float(M_PI) / 180.0f;
				child->QueryFloatAttribute("x", &mVelocity.p.x);
				child->QueryFloatAttribute("y", &mVelocity.p.y);
			}
			break;

		case 0x0dd0b0be /* "variance" */:
			{
				if (child->QueryFloatAttribute("angle", &mVariance.a) == TIXML_SUCCESS)
					mVariance.a *= float(M_PI) / 180.0f;
				child->QueryFloatAttribute("x", &mVariance.p.x);
				child->QueryFloatAttribute("y", &mVariance.p.y);
			}
			break;

		case 0x3a224d98 /* "spawn" */:
			if (const char *spawn = child->Attribute("name"))
				mSpawn = Hash(spawn);
			break;

		default:
			break;
		}
	}

	return true;
}


unsigned int SpawnTemplate::Instantiate(unsigned int aId, const Transform2 &aPosition, const Transform2 &aVelocity) const
{
	// apply position offset
	Transform2 transform(mOffset * aPosition);

	// apply position scatter
	if (mScatter.a)
		transform.a += Random::Value(0.0f, mScatter.a);
	if (mScatter.p.x)
		transform.p.x += Random::Value(0.0f, mScatter.p.x);
	if (mScatter.p.y)
		transform.p.y += Random::Value(0.0f, mScatter.p.y);

	// get local velocity
	Transform2 velocity(aVelocity.a, transform.Unrotate(aVelocity.p));

	// apply velocity inherit
	velocity.a *= mInherit.a;
	velocity.p.x *= mInherit.p.x;
	velocity.p.y *= mInherit.p.y;

	// apply velocity add
	// (TO DO: make this more straightforward)
	velocity.a += mVelocity.a;
	velocity.p.x += mVelocity.p.x;
	velocity.p.y += mVelocity.p.y;

	// apply velocity variance
	if (mVariance.a)
		velocity.a += Random::Value(0.0f, mVariance.a);
	if (mVariance.p.x)
		velocity.p.x += Random::Value(0.0f, mVariance.p.x);
	if (mVariance.p.y)
		velocity.p.y += Random::Value(0.0f, mVariance.p.y);

	// get world velocity
	velocity.p = transform.Rotate(velocity.p);

	// instantiate the item
	return Database::Instantiate(mSpawn, Database::owner.Get(aId), aId, transform.a, transform.p, velocity.p, velocity.a);
}

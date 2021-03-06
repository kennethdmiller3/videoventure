#include "StdAfx.h"

#include "Graze.h"
#include "Entity.h"
#include "Collidable.h"
#include "Link.h"
#include "Resource.h"


#ifdef USE_POOL_ALLOCATOR
// graze pool
static MemoryPool sPool(sizeof(Graze));
void *Graze::operator new(size_t aSize)
{
	return sPool.Alloc();
}
void Graze::operator delete(void *aPtr)
{
	sPool.Free(aPtr);
}
#endif


namespace Database
{
	Typed<GrazeTemplate> grazetemplate(0x9806b4d8 /* "grazetemplate" */);
	Typed<Graze *> graze(0x9086d046 /* "graze" */);

	namespace Loader
	{
		static void GrazeConfigure(unsigned int aId, const tinyxml2::XMLElement *element)
		{
			GrazeTemplate &graze = Database::grazetemplate.Open(aId);
			graze.Configure(element, aId);
			Database::grazetemplate.Close(aId);
		}
		Configure grazeconfigure(0x9086d046 /* "graze" */, GrazeConfigure);
	}

	namespace Initializer
	{
		static void GrazeActivate(unsigned int aId)
		{
			const GrazeTemplate &grazetemplate = Database::grazetemplate.Get(aId);
			Graze *graze = new Graze(grazetemplate, aId);
			Database::graze.Put(aId, graze);
			graze->Activate();
		}
		Activate grazeactivate(0x9806b4d8 /* "grazetemplate" */, GrazeActivate);

		static void GrazeDeactivate(unsigned int aId)
		{
			if (Graze *graze = Database::graze.Get(aId))
			{
				delete graze;
				Database::graze.Delete(aId);
			}
		}
		Deactivate grazedeactivate(0x9806b4d8 /* "grazetemplate" */, GrazeDeactivate);
	}
}
GrazeTemplate::GrazeTemplate(void)
: mFilter(Collidable::GetDefaultFilter())
, mOffset(0, Vector2(0, 0))
, mRadiusInner(0.0f)
, mRadiusOuter(0.0f)
, mValueInner(0.0f)
, mValueOuter(0.0f)
, mScatter(0, Vector2(0, 0))
, mInherit(0, Vector2(1, 1))
, mVelocity(0, Vector2(0, 0))
, mVariance(0, Vector2(0, 0))
, mSpawn(0)
, mSwitchOnFull(0)
{
}

GrazeTemplate::~GrazeTemplate(void)
{
}


bool GrazeTemplate::Configure(const tinyxml2::XMLElement *element, unsigned int id)
{
	if (element->QueryFloatAttribute("radius", &mRadiusOuter) == tinyxml2::XML_SUCCESS)
		mRadiusInner = mRadiusOuter * 0.5f;
	if (element->QueryFloatAttribute("value", &mValueInner) == tinyxml2::XML_SUCCESS)
		mValueOuter = 0.0f;

	ConfigureFilterData(mFilter, element);

	if (element->FirstChildElement())
	{
		for (const tinyxml2::XMLElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
		{
			switch (Hash(child->Value()))
			{
			case 0x5b9b0daf /* "ammo" */:
				if (const char *type = child->Attribute("type"))
					mType = Hash(type);
				break;

			case 0x14c8d3ca /* "offset" */:
				if (child->QueryFloatAttribute("angle", &mOffset.a) == tinyxml2::XML_SUCCESS)
					mOffset.a *= float(M_PI) / 180.0f;
				child->QueryFloatAttribute("x", &mOffset.p.x);
				child->QueryFloatAttribute("y", &mOffset.p.y);
				break;

			case 0x0dba4cb3 /* "radius" */:
				child->QueryFloatAttribute("inner", &mRadiusInner);
				child->QueryFloatAttribute("outer", &mRadiusOuter);
				break;

			case 0x425ed3ca /* "value" */:
				child->QueryFloatAttribute("inner", &mValueInner);
				child->QueryFloatAttribute("outer", &mValueOuter);
				break;

			case 0xcab7a341 /* "scatter" */:
				if (child->QueryFloatAttribute("angle", &mScatter.a) == tinyxml2::XML_SUCCESS)
					mScatter.a *= float(M_PI) / 180.0f;
				child->QueryFloatAttribute("x", &mScatter.p.x);
				child->QueryFloatAttribute("y", &mScatter.p.y);
				break;

			case 0xca04efe0 /* "inherit" */:
				if (child->QueryFloatAttribute("angle", &mInherit.a) == tinyxml2::XML_SUCCESS)
					mInherit.a *= float(M_PI) / 180.0f;
				child->QueryFloatAttribute("x", &mInherit.p.x);
				child->QueryFloatAttribute("y", &mInherit.p.y);
				break;

			case 0x32741c32 /* "velocity" */:
				if (child->QueryFloatAttribute("angle", &mVelocity.a) == tinyxml2::XML_SUCCESS)
					mVelocity.a *= float(M_PI) / 180.0f;
				child->QueryFloatAttribute("x", &mVelocity.p.x);
				child->QueryFloatAttribute("y", &mVelocity.p.y);
				break;

			case 0x0dd0b0be /* "variance" */:
				if (child->QueryFloatAttribute("angle", &mVariance.a) == tinyxml2::XML_SUCCESS)
					mVariance.a *= float(M_PI) / 180.0f;
				child->QueryFloatAttribute("x", &mVariance.p.x);
				child->QueryFloatAttribute("y", &mVariance.p.y);
				break;

			case 0x3a224d98 /* "spawn" */:
				if (const char *spawn = child->Attribute("name"))
					mSpawn = Hash(spawn);
				break;

			default:
				continue;
			}
		}
	}

	if (const char *spawn = element->Attribute("switchonfull"))
		mSwitchOnFull = Hash(spawn);

	return true;
}


Graze::Graze(void)
: Updatable(0)
, mAmmo(0)
{
}

Graze::Graze(const GrazeTemplate &aTemplate, unsigned int aId)
: Updatable(aId)
, mAmmo(0)
{
	// set updatable action
	SetAction(Updatable::Action(this, &Graze::Update));

	// if the graze collects ammo...
	if (aTemplate.mType)
	{
		// find the specified resource
		mAmmo = FindResourceContainer(aId, aTemplate.mType);
	}
}

Graze::~Graze(void)
{
}

class GrazeQueryCallback
{
public:
	unsigned int mId;
	GrazeTemplate mGraze;
	Transform2 mTransform;
	unsigned int mAmmo;
	float mStep;

public:
	void Report(CollidableShape *shape, float distance, const Vector2 &point)
	{
		// skip unhittable shapes
		if (Collidable::IsSensor(shape))
			return;
		if (!Collidable::CheckFilter(mGraze.mFilter, Collidable::GetFilter(shape)))
			return;

		// get the collidable identifier
		unsigned int targetId = Collidable::GetId(shape);

		// skip non-entity
		if (targetId == 0)
			return;

		// skip self
		if (targetId == mId)
			return;

		// get center position
		Vector2 centerPos(Collidable::GetCenter(shape));

		// get direction
		Vector2 dir(mTransform.Transform(centerPos));

		// apply value falloff
		float interp;
		if (distance <= mGraze.mRadiusInner)
			interp = 0.0f;
		else
			interp = (distance - mGraze.mRadiusInner) / (mGraze.mRadiusOuter - mGraze.mRadiusInner);
		float value = Lerp(mGraze.mValueInner, mGraze.mValueOuter, interp);

		// if spawning on collect...
		if (mGraze.mSpawn)
		{
			// spawn a spark
			Spark(targetId);
		}

		// ammo resource (if any)
		Resource *resource = Database::resource.Get(mAmmo).Get(mGraze.mType);
		if (resource)
		{
			// add value
			resource->Add(mId, value * mStep);

			// if switching on full...
			if (mGraze.mSwitchOnFull)
			{
				// ammo resource template
				const ResourceTemplate &resourcetemplate = Database::resourcetemplate.Get(mAmmo).Get(mGraze.mType);

				// if full...
				if (resource->GetValue() >= resourcetemplate.mMaximum)
				{
					// save spillover ammo
					//float spillover = resource->GetValue() - resourcetemplate.mMaximum;

					// change dynamic type
					Database::Switch(mId, mGraze.mSwitchOnFull);
				}
			}
		}
	}

	void Spark(unsigned int aId)
	{
		// TO DO: consolidate this with similar spawn patterns (Spawner, Weapon)

		// get the source
		Entity *entity = Database::entity.Get(aId);

		// get world transform
		Transform2 transform(entity->GetTransform());

		// apply transform scatter
		if (mGraze.mScatter.a)
			transform.a += Random::Value(0.0f, mGraze.mScatter.a);
		if (mGraze.mScatter.p.x)
			transform.p.x += Random::Value(0.0f, mGraze.mScatter.p.x);
		if (mGraze.mScatter.p.y)
			transform.p.y += Random::Value(0.0f, mGraze.mScatter.p.y);

		// get local velocity
		Transform2 velocity(entity->GetOmega(), transform.Unrotate(entity->GetVelocity()));

		// apply velocity inherit
		velocity.a *= mGraze.mInherit.a;
		velocity.p.x *= mGraze.mInherit.p.x;
		velocity.p.y *= mGraze.mInherit.p.y;

		// apply velocity add
		velocity.a += mGraze.mVelocity.a;
		velocity.p.x += mGraze.mVelocity.p.x;
		velocity.p.y += mGraze.mVelocity.p.y;

		// apply velocity variance
		if (mGraze.mVariance.a)
			velocity.a += Random::Value(0.0f, mGraze.mVariance.a);
		if (mGraze.mVariance.p.x)
			velocity.p.x += Random::Value(0.0f, mGraze.mVariance.p.x);
		if (mGraze.mVariance.p.y)
			velocity.p.y += Random::Value(0.0f, mGraze.mVariance.p.y);

		// get world velocity
		velocity.p = transform.Rotate(velocity.p);

		// instantiate the spawn entity
		Database::Instantiate(mGraze.mSpawn, Database::owner.Get(mId), mId, transform.a, transform.p, velocity.p, velocity.a, true);
	}
};

void Graze::Update(float aStep)
{
	// get parent entity
	Entity *entity = Database::entity.Get(mId);

	// get the graze template
	const GrazeTemplate &graze = Database::grazetemplate.Get(mId);

	// set up query callback
	GrazeQueryCallback callback;
	callback.mId = mId;
	callback.mGraze = graze;
	callback.mTransform = (entity->GetTransform() * graze.mOffset).Inverse();
	callback.mAmmo = mAmmo;
	callback.mStep = aStep;

	// get nearby shapes
	Collidable::QueryRadius(entity->GetPosition(), graze.mRadiusOuter, graze.mFilter, 
		Collidable::QueryRadiusDelegate(&callback, &GrazeQueryCallback::Report));
}

#include "StdAfx.h"
#include "Capture.h"
#include "Controller.h"
#include "Collidable.h"
#include "Capturable.h"
#include "Link.h"
#include "Team.h"
#include "Sound.h"


#ifdef USE_POOL_ALLOCATOR
#include <boost/pool/pool.hpp>

// capture pool
static boost::pool<boost::default_user_allocator_malloc_free> pool(sizeof(Capture));
void *Capture::operator new(size_t aSize)
{
	return pool.malloc();
}
void Capture::operator delete(void *aPtr)
{
	pool.free(aPtr);
}
#endif


namespace Database
{
	Typed<CaptureTemplate> capturetemplate(0x6d1fb927 /* "capturetemplate" */);
	Typed<Capture *> capture(0xfc9819c1 /* "capture" */);
	
	namespace Loader
	{
		class CaptureLoader
		{
		public:
			CaptureLoader()
			{
				AddConfigure(0xfc9819c1 /* "capture" */, Entry(this, &CaptureLoader::Configure));
			}

			void Configure(unsigned int aId, const TiXmlElement *element)
			{
				CaptureTemplate &capture = Database::capturetemplate.Open(aId);
				capture.Configure(element, aId);
				Database::capturetemplate.Close(aId);
			}
		}
		captureloader;
	}

	namespace Initializer
	{
		class CaptureInitializer
		{
		public:
			CaptureInitializer()
			{
				AddActivate(0x6d1fb927 /* "capturetemplate" */, Entry(this, &CaptureInitializer::Activate));
				AddDeactivate(0x6d1fb927 /* "capturetemplate" */, Entry(this, &CaptureInitializer::Deactivate));
			}

			void Activate(unsigned int aId)
			{
				const CaptureTemplate &capturetemplate = Database::capturetemplate.Get(aId);
				Capture *capture = new Capture(capturetemplate, aId);
				Database::capture.Put(aId, capture);

				// TO DO: check to make sure this does not have an order dependency
				capture->SetControl(aId);
				for (unsigned int aControlId = aId; aControlId != 0; aControlId = Database::backlink.Get(aControlId))
				{
					if (Database::controller.Find(aControlId))
					{
						capture->SetControl(aControlId);
						break;
					}
				}

				capture->Activate();
			}

			void Deactivate(unsigned int aId)
			{
				if (Capture *capture = Database::capture.Get(aId))
				{
					delete capture;
					Database::capture.Delete(aId);
				}
			}
		}
		captureinitializer;
	}
};


CaptureTemplate::CaptureTemplate(void)
: mEnergy(1.0f)
, mConsume(0.0f)
, mRecover(0.0f)
, mStrength(0.0f)
, mRadius(0.0f)
, mAngle(0.0f)
, mChannel(0)
{
}

CaptureTemplate::~CaptureTemplate(void)
{
}


bool CaptureTemplate::Configure(const TiXmlElement *element, unsigned int id)
{
	element->QueryFloatAttribute("energy", &mEnergy);
	element->QueryFloatAttribute("consume", &mConsume);
	element->QueryFloatAttribute("recover", &mRecover);

	element->QueryFloatAttribute("strength", &mStrength);
	element->QueryFloatAttribute("radius", &mRadius);
	if (element->QueryFloatAttribute("angle", &mAngle) == TIXML_SUCCESS)
		mAngle *= float(M_PI) / 180.0f;

	if (element->QueryIntAttribute("channel", &mChannel) == TIXML_SUCCESS)
		--mChannel;

	return true;
}


Capture::Capture(void)
: Updatable(0)
, mControlId(0)
, mChannel(0)
, mEnergy(0)
, mActive(false)
{
	SetAction(Action(this, &Capture::Update));
}

Capture::Capture(const CaptureTemplate &aTemplate, unsigned int aId)
: Updatable(aId)
, mControlId(0)
, mChannel(aTemplate.mChannel)
, mEnergy(aTemplate.mEnergy)
, mActive(true)
{
	SetAction(Action(this, &Capture::Update));
}

Capture::~Capture(void)
{
}

void Capture::Update(float aStep)
{
	// get controller
	const Controller *controller = Database::controller.Get(mControlId);
	if (!controller)
		return;

	// get the template
	const CaptureTemplate &capture = Database::capturetemplate.Get(mId);

	// if triggered, and enough energy to fire...
	if (controller->mFire[mChannel] && mEnergy > 0.0f)
	{
		// consume energy
		mEnergy -= aStep * capture.mConsume;

		// start effects
		Start();

		// get parent entity
		Entity *entity = Database::entity.Get(mId);

		// get the collision world
		b2World *world = Collidable::GetWorld();

		// get nearby shapes
		// TO DO: optimize for angle
		b2AABB aabb;
		const float lookRadius = capture.mRadius;
		aabb.lowerBound.Set(entity->GetPosition().x - lookRadius, entity->GetPosition().y - lookRadius);
		aabb.upperBound.Set(entity->GetPosition().x + lookRadius, entity->GetPosition().y + lookRadius);
		b2Shape* shapes[b2_maxProxies];
		int32 count = world->Query(aabb, shapes, b2_maxProxies);

		// get team affiliation
		unsigned int aTeam = Database::team.Get(mId);

		// world-to-local transform
		Matrix2 transform(entity->GetTransform().Inverse());

		// for each shape...
		for (int32 i = 0; i < count; ++i)
		{
			// get the parent body
			b2Body* body = shapes[i]->GetBody();

			// get the collidable identifier
			unsigned int targetId = reinterpret_cast<unsigned int>(body->GetUserData());

			// skip non-entity
			if (targetId == 0)
				continue;

			// skip self
			if (targetId == mId)
				continue;

			// get team affiliation
			unsigned int targetTeam = Database::team.Get(targetId);

			// skip teammate
			if (targetTeam == aTeam)
				continue;

			// get range
			Vector2 dir(transform.Transform(Vector2(body->GetPosition())));
			float range = dir.Length() - 0.5f * shapes[i]->GetSweepRadius();

			// skip if out of range
			if (range > capture.mRadius)
				continue;

			// skip if outside angle
			// TO DO: handle partial overlap
			if (fabsf(atan2f(dir.x, dir.y)) > capture.mAngle)
				continue;

			// if the recipient is capturable...
			Capturable *capturable = Database::capturable.Get(targetId);
			if (capturable)
			{
				// get base strength
				float strength = capture.mStrength * aStep;

				// apply strength falloff
				if (range > 0)
				{
					strength *= (1.0f - (range * range) / (capture.mRadius * capture.mRadius));
				}

				// limit healing
				if (strength < 0)
				{
					strength = std::max(strength, capturable->GetResistance() - Database::capturabletemplate.Get(targetId).mResistance);
				}

				// apply strength
				capturable->Persuade(mId, strength);
			}
		}
	}
	else
	{
		// stop effects
		Stop();
	}

	// recover energy
	mEnergy = std::min(mEnergy + aStep * capture.mRecover, capture.mEnergy);
}

// start effects
void Capture::Start(void)
{
	// skip if already active
	if (mActive)
		return;

	// set as active
	mActive = true;

	// show the renderable
	if (Renderable *renderable = Database::renderable.Get(mId))
		renderable->Show();

	// start the sound cue
	PlaySound(mId, 0x8eab16d9 /* "fire" */);
}

// stop effects
void Capture::Stop(void)
{
	// skip if already inactive
	if (!mActive)
		return;

	// set as inactive
	mActive = false;

	// hide the renderable
	if (Renderable *renderable = Database::renderable.Get(mId))
		renderable->Hide();

	// stop the sound cue
	StopSound(mId, 0x8eab16d9 /* "fire" */);
}
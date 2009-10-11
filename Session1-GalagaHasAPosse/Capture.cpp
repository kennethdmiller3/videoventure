#include "StdAfx.h"
#include "Capture.h"
#include "Entity.h"
#include "Controller.h"
#include "Collidable.h"
#include "Capturable.h"
#include "Link.h"
#include "Team.h"
#include "Renderable.h"
#include "Sound.h"
#undef max
#undef min

#ifdef USE_POOL_ALLOCATOR
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

class CaptureQueryCallback : public b2QueryCallback
{
public:
	unsigned int mId;
	CaptureTemplate mCapture;
	Transform2 mTransform;
	int mTeam;
	float mCurRadius[2];
	float mCurStrength[2];

public:
	virtual bool ReportFixture(b2Fixture* fixture)
	{
		// skip unhittable fixtures
		if (fixture->IsSensor())
			return true;

		// get the parent body
		b2Body* body = fixture->GetBody();

		// get the collidable identifier
		unsigned int targetId = reinterpret_cast<unsigned int>(body->GetUserData());

		// skip non-entity
		if (targetId == 0)
			return true;

		// skip self
		if (targetId == mId)
			return true;

		// get team affiliation
		unsigned int targetTeam = Database::team.Get(targetId);

		// skip teammate
		if (targetTeam == mTeam)
			return true;

		// get local position
		b2Vec2 localPos;
		switch (fixture->GetType())
		{
		case b2Shape::e_circle:		localPos = static_cast<b2CircleShape *>(fixture->GetShape())->m_p;	break;
		case b2Shape::e_polygon:	localPos = static_cast<b2PolygonShape *>(fixture->GetShape())->m_centroid; break;
		default:					localPos = Vector2(0, 0); break;
		}
		Vector2 fixturePos(body->GetWorldPoint(localPos));

		// get range
		Vector2 dir(mTransform.Transform(fixturePos));
		float range = dir.Length();
		float radius = 0.5f * fixture->GetShape()->m_radius;	//fixture->ComputeSweepRadius(localPos);

		// skip if out of range
		if (range > mCurRadius[1] + radius)
			return true;

		// skip if outside angle
		// TO DO: handle partial overlap
		if (fabsf(atan2f(dir.x, dir.y)) > mCapture.mAngle)
			return true;

		// apply strength falloff
		float strength;
		if (range <= mCurRadius[0] - radius)
		{
			strength = mCurStrength[0];
		}
		else
		{
			float interp = (range - mCurRadius[0] + radius) / (mCurRadius[1] + radius - mCurRadius[0] + radius);
			strength = Lerp(mCurStrength[0], mCurStrength[1], interp);
		}

		// if the recipient is capturable...
		// and not healing or the target is at max resistance...
		if (Capturable *capturable = Database::capturable.Get(targetId))
		{
			// limit healing
			if (strength < 0)
			{
				strength = std::max(strength, capturable->GetResistance() - Database::capturabletemplate.Get(targetId).mResistance);
			}

			// apply strength
			capturable->Persuade(mId, strength);
		}

		return true;
	}
};

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

		// set up query callback
		CaptureQueryCallback callback;
		callback.mId = mId;
		callback.mCapture = capture;
		callback.mTransform = entity->GetTransform().Inverse();
		callback.mTeam = Database::team.Get(mId);

		// default radius and strength
		callback.mCurRadius[0] = 0.0f;
		callback.mCurRadius[1] = capture.mRadius;
		callback.mCurStrength[0] = capture.mStrength * aStep;
		callback.mCurStrength[1] = 0.0f;

		// get nearby fixtures
		// TO DO: optimize for angle
		b2AABB aabb;
		const float lookRadius = callback.mCurRadius[1];
		aabb.lowerBound.Set(entity->GetPosition().x - lookRadius, entity->GetPosition().y - lookRadius);
		aabb.upperBound.Set(entity->GetPosition().x + lookRadius, entity->GetPosition().y + lookRadius);
		Collidable::QueryAABB(&callback, aabb);
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
	PlaySoundCue(mId, 0x8eab16d9 /* "fire" */);
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
	StopSoundCue(mId, 0x8eab16d9 /* "fire" */);
}
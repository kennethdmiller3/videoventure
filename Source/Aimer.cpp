#include "StdAfx.h"
#include "Aimer.h"
#include "Entity.h"
#include "Collidable.h"
#include "Link.h"
#include "Weapon.h"
#include "Damagable.h"
#include "Team.h"

#include "Ship.h"

#include "Behavior/BotUtilities.h"
#include "Behavior/WanderBehavior.h"
#include "Behavior/TargetBehavior.h"
#include "Behavior/PursueBehavior.h"
#include "Behavior/AimBehavior.h"
#include "Behavior/EvadeBehavior.h"
#include "Behavior/RangeBehavior.h"
#include "Behavior/EdgeBehavior.h"


#ifdef USE_POOL_ALLOCATOR
// aimer pool
static boost::pool<boost::default_user_allocator_malloc_free> pool(sizeof(Aimer));
void *Aimer::operator new(size_t aSize)
{
	return pool.malloc();
}
void Aimer::operator delete(void *aPtr)
{
	pool.free(aPtr);
}
#endif


namespace Database
{
	Typed<AimerTemplate> aimertemplate(0x9bde0ae7 /* "aimertemplate" */);
	Typed<Aimer *> aimer(0x2ea90881 /* "aimer" */);

	namespace Loader
	{
		class AimerLoader
		{
		public:
			AimerLoader()
			{
				AddConfigure(0x2ea90881 /* "aimer" */, Entry(this, &AimerLoader::Configure));
			}

			void Configure(unsigned int aId, const TiXmlElement *element)
			{
				AimerTemplate &aimer = Database::aimertemplate.Open(aId);
				aimer.Configure(element, aId);
				Database::aimertemplate.Close(aId);
			}
		}
		aimerloader;
	}

	namespace Initializer
	{
		class AimerInitializer
		{
		public:
			AimerInitializer()
			{
				AddActivate(0x9bde0ae7 /* "aimertemplate" */, Entry(this, &AimerInitializer::Activate));
				AddDeactivate(0x9bde0ae7 /* "aimertemplate" */, Entry(this, &AimerInitializer::Deactivate));
			}

			void Activate(unsigned int aId)
			{
				const AimerTemplate &aimertemplate = Database::aimertemplate.Get(aId);
				Aimer *aimer = new Aimer(aimertemplate, aId);
				Database::aimer.Put(aId, aimer);
				Database::controller.Put(aId, aimer);
				aimer->Activate();
			}

			void Deactivate(unsigned int aId)
			{
				if (Aimer *aimer = Database::aimer.Get(aId))
				{
					delete aimer;
					Database::aimer.Delete(aId);
					Database::controller.Delete(aId);
				}
			}
		}
		aimerinitializer;
	}
}

AimerTemplate::AimerTemplate(void)
: mDrift(0.0f)
{
}

AimerTemplate::~AimerTemplate(void)
{
}

bool AimerTemplate::Configure(const TiXmlElement *element, unsigned int aId)
{
	for (const TiXmlElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
	{
		switch (Hash(child->Value()))
		{
		case 0x2e87eea4 /* "drift" */:
			{
				child->QueryFloatAttribute("strength", &mDrift);
			}
			break;

		case 0xf23b7114 /* "wander" */:
			{
				WanderBehaviorTemplate &wander = Database::wanderbehaviortemplate.Open(aId);
				wander.Configure(child, aId);
				Database::wanderbehaviortemplate.Close(aId);
			}
			break;

		case 0x32608848 /* "target" */:
			{
				TargetBehaviorTemplate &targetbehavior = Database::targetbehaviortemplate.Open(aId);
				targetbehavior.Configure(child, aId);
				Database::targetbehaviortemplate.Close(aId);
			}
			break;

		case 0x0297228f /* "pursue" */:
			{
				PursueBehaviorTemplate &pursuebehavior = Database::pursuebehaviortemplate.Open(aId);
				pursuebehavior.Configure(child, aId);
				Database::pursuebehaviortemplate.Close(aId);
			}
			break;

		case 0x383251f6 /* "aim" */:
			{
				AimBehaviorTemplate &aim = Database::aimbehaviortemplate.Open(aId);
				aim.Configure(child, aId);
				Database::aimbehaviortemplate.Close(aId);
			}
			break;

		case 0x8eab16d9 /* "fire" */:
			{
				Database::Typed<FireConeTemplate> &firebehaviors = Database::fireconetemplate.Open(aId);
				const char *name = child->Attribute("name");
				unsigned int aSubId = name ? Hash(name) : firebehaviors.GetCount() + 1;
				FireConeTemplate &firebehavior = firebehaviors.Open(aSubId);
				firebehavior.Configure(child, aId);
				firebehaviors.Close(aSubId);
				Database::fireconetemplate.Close(aId);
			}
			break;

		case 0x3cf27f66 /* "evade" */:
			{
				EvadeBehaviorTemplate &evadebehavior = Database::evadebehaviortemplate.Open(aId);
				evadebehavior.Configure(child, aId);
				Database::evadebehaviortemplate.Close(aId);
			}
			break;

		case 0x27cb3b23 /* "close" */:
			{
				CloseBehaviorTemplate &closebehavior = Database::closebehaviortemplate.Open(aId);
				closebehavior.Configure(child, aId);
				Database::closebehaviortemplate.Close(aId);
			}
			break;

		case 0xbcf819ee /* "far" */:
			{
				FarBehaviorTemplate &farbehavior = Database::farbehaviortemplate.Open(aId);
				farbehavior.Configure(child, aId);
				Database::farbehaviortemplate.Close(aId);
			}
			break;

		case 0x56f6d83c /* "edge" */:
			{
				EdgeBehaviorTemplate &edgebehavior = Database::edgebehaviortemplate.Open(aId);
				edgebehavior.Configure(child, aId);
				Database::edgebehaviortemplate.Close(aId);
			}
			break;
		}
	}
	return true;
}


Aimer::Aimer(const AimerTemplate &aTemplate, unsigned int aId)
: Controller(aId)
, Brain()
{
	SetAction(Action(this, &Aimer::Control));

	// TO DO: replace this hard-wired behavior with a Loader-based system

	const WanderBehaviorTemplate &wanderbehavior = Database::wanderbehaviortemplate.Get(aId);
	if (wanderbehavior.mFront || wanderbehavior.mSide || wanderbehavior.mTurn)
	{
		mBehaviors.push_back(new WanderBehavior(aId, wanderbehavior, this));
		mScheduler.Run(*mBehaviors.back());
	}

	const TargetBehaviorTemplate &target = Database::targetbehaviortemplate.Get(aId);
	if (target.mRange)
	{
		mBehaviors.push_back(new TargetBehavior(aId, target, this));
		mScheduler.Run(*mBehaviors.back());
	}

	const PursueBehaviorTemplate &pursuebehavior = Database::pursuebehaviortemplate.Get(aId);
	if (pursuebehavior.mStrength)
	{
		mBehaviors.push_back(new PursueBehavior(aId, pursuebehavior, this));
		mScheduler.Run(*mBehaviors.back());
	}

	const AimBehaviorTemplate &aimbehavior = Database::aimbehaviortemplate.Get(aId);
	if (aimbehavior.mStrength)
	{
		mBehaviors.push_back(new AimBehavior(aId, aimbehavior, this));
		mScheduler.Run(*mBehaviors.back());
	}

	const EvadeBehaviorTemplate &evadebehavior = Database::evadebehaviortemplate.Get(aId);
	if (evadebehavior.mStrength)
	{
		mBehaviors.push_back(new EvadeBehavior(aId, evadebehavior, this));
		mScheduler.Run(*mBehaviors.back());
	}

	const CloseBehaviorTemplate &closebehavior = Database::closebehaviortemplate.Get(aId);
	const FarBehaviorTemplate &farbehavior = Database::farbehaviortemplate.Get(aId);
	if (closebehavior.mRange > 0.0f || farbehavior.mRange < FLT_MAX)
	{
		mBehaviors.push_back(new RangeBehavior(aId, this));
		mScheduler.Run(*mBehaviors.back());
	}

	const EdgeBehaviorTemplate &edgebehavior = Database::edgebehaviortemplate.Get(aId);
	if (edgebehavior.mStrength != 0.0f)
	{
		mBehaviors.push_back(new EdgeBehavior(aId, this));
		mScheduler.Run(*mBehaviors.back());
	}
}

Aimer::~Aimer(void)
{
	mScheduler.Stop();
	for (std::vector<Behavior *>::iterator i = mBehaviors.begin(); i != mBehaviors.end(); ++i)
		delete (*i);
}

// Aimer Control
void Aimer::Control(float aStep)
{
	// get parent entity
	Entity *entity = Database::entity.Get(mId);

	// get transform
	const Transform2 &transform = entity->GetTransform();

	// get aimer template
	const AimerTemplate &aimer = Database::aimertemplate.Get(mId);

	// set default controls
	mMove = transform.Rotate(Vector2(0, aimer.mDrift));
	mAim = Vector2(0, 0);
	mTurn = 0;
	memset(mFire, 0, sizeof(mFire));

	// think
	Think(aStep);

#ifdef AIMER_OBSTACLE_AVOIDANCE
	// obstacle avoidance
	if (Collidable *collidable = Database::collidable.Get(mId))
	{
		b2Body *body = collidable->GetBody();
		b2Shape *shapelist = body->GetShapeList();
		const b2FilterData &filter = shapelist->GetFilterData();

		// collision probe
		b2Segment segment;
		segment.p1 = transform.p;
		segment.p2 = transform.p + 32 * mMove;

		// perform a segment test
		float lambda = 1.0f;
		b2Vec2 normal(0, 0);
		b2Shape *shape = NULL;
		Collidable::TestSegment(segment, shapelist->GetSweepRadius() * 0.5f, mId, filter.categoryBits, filter.maskBits, lambda, normal, shape);
		if (lambda < 1.0f)
		{
			float push = 1.0f - lambda;
			mMove += push * normal;
			mTurn += transform.y.Cross(normal) > 0.0f ? push : -push;
		}
	}
#endif

	// limit move to 100%
	float moveLengthSq = mMove.LengthSq();
	if (moveLengthSq > 1.0f)
		mMove *= InvSqrt(moveLengthSq);

#ifdef AIMER_DEBUG_DRAW_CONTROLS
	glColor4f(0.0f, 0.0f, 1.0f, 1.0f);
	glBegin(GL_LINES);
	glVertex2f(transform.p.x, transform.p.y);
	glVertex2f(transform.p.x + 16 * mMove.x, transform.p.y + 16 * mMove.y);
	glEnd();
#endif

	// convert to local coordinates
	mMove = transform.Unrotate(mMove);

	// limit turn to 100%
	mTurn = Clamp(mTurn, -1.0f, 1.0f);

#ifdef AIMER_DEBUG_DRAW_CONTROLS
	glColor4f(1.0f, 0.0f, 1.0f, 1.0f);
	glBegin(GL_LINE_STRIP);
	glVertex2f(transform.p.x, transform.p.y);
	int steps = xs_CeilToInt(mTurn * 16);
	for(int i = 0; i < steps; i++)
	{
		float angle = float(M_PI) * i * mTurn / steps;
		Vector2 dir = transform.Transform(Vector2(-16*sinf(angle), 16*cosf(angle)));
		glVertex2f(dir.x, dir.y);
	}
	glEnd();
#endif
}

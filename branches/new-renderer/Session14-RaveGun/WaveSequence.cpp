#include "StdAfx.h"
#include "WaveSequence.h"
#include "Entity.h"
#include "Renderable.h"
#include "Team.h"
#include "Spawn.h"
#include "ExpressionConfigure.h"
#include "Sound.h"

/*
WaveSequence
{
	Pre-Sequence Delay
	Wave[]
	{
		Pre-Wave Delay
		Spawn Entities
		Track Entities
		Post-Wave Delay
	}
	Post-Sequence Delay
}
*/

#ifdef USE_POOL_ALLOCATOR
// wavesequence pool
static boost::pool<boost::default_user_allocator_malloc_free> pool(sizeof(WaveSequence));
void *WaveSequence::operator new(size_t aSize)
{
	return pool.malloc();
}
void WaveSequence::operator delete(void *aPtr)
{
	pool.free(aPtr);
}
#endif


class WaveSequenceTracker
{
public:
	unsigned int mId;

	WaveSequenceTracker(unsigned int aId = 0)
		: mId(aId)
	{
		if (WaveSequence *wavesequence = Database::wavesequence.Get(mId))
			wavesequence->Track(1);
	}

	WaveSequenceTracker(const WaveSequenceTracker &aSource)
		: mId(aSource.mId)
	{
		if (WaveSequence *wavesequence = Database::wavesequence.Get(mId))
			wavesequence->Track(1);
	}

	~WaveSequenceTracker()
	{
		if (WaveSequence *wavesequence = Database::wavesequence.Get(mId))
			wavesequence->Track(-1);
	}

	const WaveSequenceTracker &operator=(const WaveSequenceTracker &aSource)
	{
		if (WaveSequence *wavesequence = Database::wavesequence.Get(mId))
			wavesequence->Track(-1);
		if (WaveSequence *wavesequence = Database::wavesequence.Get(mId))
			wavesequence->Track(1);
		return *this;
	}
};


namespace Database
{
	Typed<WaveSequenceTemplate> wavesequencetemplate(0x684715cf /* "wavesequencetemplate" */);
	Typed<WaveSequence *> wavesequence(0x943e7789 /* "wavesequence" */);
	Typed<WaveSequenceTracker> wavesequencetracker(0xfebb6857 /* "wavesequencetracker" */);

	namespace Loader
	{
		class WaveSequenceLoader
		{
		public:
			WaveSequenceLoader()
			{
				AddConfigure(0x943e7789 /* "wavesequence" */, Entry(this, &WaveSequenceLoader::Configure));
			}

			void Configure(unsigned int aId, const TiXmlElement *element)
			{
				WaveSequenceTemplate &wavesequence = Database::wavesequencetemplate.Open(aId);
				wavesequence.Configure(element, aId);
				Database::wavesequencetemplate.Close(aId);
			}
		}
		wavesequenceloader;
	}

	namespace Initializer
	{
		class WaveSequenceInitializer
		{
		public:
			WaveSequenceInitializer()
			{
				AddActivate(0x684715cf /* "wavesequencetemplate" */, Entry(this, &WaveSequenceInitializer::Activate));
				AddDeactivate(0x684715cf /* "wavesequencetemplate" */, Entry(this, &WaveSequenceInitializer::Deactivate));
			}

			void Activate(unsigned int aId)
			{
				const WaveSequenceTemplate &wavesequencetemplate = Database::wavesequencetemplate.Get(aId);
				WaveSequence *wavesequence = new WaveSequence(wavesequencetemplate, aId);
				Database::wavesequence.Put(aId, wavesequence);
				wavesequence->Activate();
			}

			void Deactivate(unsigned int aId)
			{
				if (WaveSequence *wavesequence = Database::wavesequence.Get(aId))
				{
					delete wavesequence;
					Database::wavesequence.Delete(aId);
				}
			}
		}
		wavesequenceinitializer;
	}
}


// actions

static const char * const sTransformNames[] = { "x", "y", "angle", ""};
static const float sTransformDefault[] = { 0.0f, 0.0f, 0.0f, 0.0f };

template<> inline Transform2 Cast<Transform2, __m128>(__m128 i)
{
	return Transform2(reinterpret_cast<const float * __restrict>(&i)[2],
		Vector2(reinterpret_cast<const float * __restrict>(&i)[0], reinterpret_cast<const float * __restrict>(&i)[1]));
}

static void WaveWait(EntityContext &aContext)
{
	float delay = Expression::Evaluate<float>(aContext);
	aContext.mParam -= delay;
}

static void WaveSpawn(EntityContext &aContext)
{
	// get parameters
	unsigned int spawn(Expression::Read<unsigned int>(aContext));
	int track(Expression::Read<unsigned int>(aContext));
	Transform2 position(Cast<Transform2, __m128>(Expression::Evaluate<__m128>(aContext)));
	position.a *= float(M_PI) / 180.0f;
	Transform2 velocity(Cast<Transform2, __m128>(Expression::Evaluate<__m128>(aContext)));
	velocity.a *= float(M_PI) / 180.0f;

	// get the entity
	Entity *entity = Database::entity.Get(aContext.mId);

	// interpolated transform
	Transform2 basetransform(entity->GetInterpolatedTransform(aContext.mParam / sim_step));

	// get world position
	position *= basetransform;

	// get world velocity
	velocity.p = position.Rotate(velocity.p);

	// instantiate a entity
	if (unsigned int ordId = Database::Instantiate(spawn, Database::owner.Get(aContext.mId), aContext.mId, position.a, position.p, velocity.p, velocity.a))
	{
		// set fractional turn
		if (Renderable *renderable = Database::renderable.Get(ordId))
			renderable->SetFraction(aContext.mParam / sim_step);

		// if tracking....
		if (track)
		{
			// add a tracker
			Database::wavesequencetracker.Put(ordId, WaveSequenceTracker(aContext.mId));
		}
	}
}

static void WaveSound(EntityContext &aContext)
{
	unsigned int name(Expression::Read<unsigned int>(aContext));
	PlaySoundCue(aContext.mId, name);
}

void WaveStartRepeat(EntityContext &aContext)
{
	// initialize repeat count and sequence offset
	unsigned int name(Expression::Read<unsigned int>(aContext));
	aContext.mVars->Put(name, Expression::Evaluate<float>(aContext));
	aContext.mVars->Put(name+1, Cast<float, unsigned int>(0));
}

void WaveRepeat(EntityContext &aContext)
{
	const unsigned int *reset = aContext.mStream - 1;

	// get repeat count and sequence offset
	unsigned int name(Expression::Read<unsigned int>(aContext));
	float count = aContext.mVars->Get(name);
	unsigned int offset = Cast<unsigned int, float>(aContext.mVars->Get(name+1));

	// get repeat block
	size_t size(Expression::Read<size_t>(aContext));
	const unsigned int *begin = aContext.mStream;
	const unsigned int *end = aContext.mStream + size;

	// resume from saved offset
	aContext.mStream += offset;

	// while iterations left...
	while (count > 0.0f)
	{
		// evaluate the next expression
		Expression::Evaluate<void>(aContext);

		// if reaching the end...
		if (aContext.mStream >= end)
		{
			// rewind and decrement the count
			aContext.mStream = begin;
			count -= 1;
			if (count <= 0.0f)
				break;
		}

		// if out of time...
		if (aContext.mParam < -0.0001f)
		{
			// save repeat count and sequence offset
			aContext.mVars->Put(name, count);
			aContext.mVars->Put(name+1, Cast<float, unsigned int>(aContext.mStream - begin));
			aContext.mStream = reset;
			return;
		}
	}

	// clear out variables
	aContext.mVars->Delete(name);
	aContext.mVars->Delete(name+1);

	// jump to the end
	aContext.mStream = end;
}

// configue a parameter
static void ConfigureParameter(const TiXmlElement *element, const char *param, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[])
{
	// get constant value from attribute
	float value = defaults[0];
	element->QueryFloatAttribute(param, &value);

	// if there is a child tag for the parameter...
	if (const TiXmlElement *child = element->FirstChildElement(param))
	{
		// configure the expression
		ConfigureExpressionRoot<float>(child, buffer, names, &value);
	}
	else
	{
		// append a constant expression
		Expression::Append(buffer, Expression::Constant<float>, value);
	}
}

bool WaveTemplate::ConfigureAction(const TiXmlElement *element, unsigned int aId)
{
	// process child elements
	for (const TiXmlElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
	{
		unsigned int aPropId = Hash(child->Value());
		switch (aPropId)
		{
		case 0x892e4ca0 /* "wait" */:
			{
				Expression::Append(mAction, WaveWait);
				ConfigureExpressionRoot<float>(child, mAction, sScalarNames, sScalarDefault);
			}
			break;

		case 0xc4642eff /* "action" */:
			{
				// reserve size
				mAction.push_back(0);
				int start = mAction.size();

				// configure actions
				ConfigureAction(child, aId);

				// set size
				mAction[start-1] = mAction.size() - start;
			}
			break;

		case 0xd99ba82a /* "repeat" */:
			{
				// generate an identifier based on offset
				size_t id = mAction.size() * 4;

				// configure loop setup
				Expression::Append(mAction, WaveStartRepeat, id);
				ConfigureParameter(child, "count", mAction, sScalarNames, sScalarDefault);

				// append loop repeat
				Expression::Append(mAction, WaveRepeat, id);

				// configure loop body
				mAction.push_back(0);
				int start = mAction.size();
				ConfigureAction(child, aId);
				mAction[start-1] = mAction.size() - start;
			}
			break;

		case 0x3a224d98 /* "spawn" */:
			Expression::Append(mAction, WaveSpawn, Hash(child->Attribute("name")), mTrack);
			if (const TiXmlElement *param = child->FirstChildElement("position"))
				ConfigureExpressionRoot<__m128>(param, mAction, sTransformNames, sTransformDefault);
			else
				Expression::Append(mAction, Expression::Constant<__m128>, _mm_setzero_ps());
			if (const TiXmlElement *param = child->FirstChildElement("velocity"))
				ConfigureExpressionRoot<__m128>(param, mAction, sTransformNames, sTransformDefault);
			else
				Expression::Append(mAction, Expression::Constant<__m128>, _mm_setzero_ps());
			break;

		case 0xe5561300 /* "cue" */:
			Expression::Append(mAction, WaveSound, Hash(child->Attribute("name")));
			break;

#if 0
		case 0xd99ba82a /* "repeat" */:
			{
				int count = 1;
				child->QueryIntAttribute("count", &count);

				Expression::Append(mAction, Expression::Repeat, count);

				mAction.push_back(0);
				int start = mAction.size();
				ConfigureAction(child, aId);
				mAction[start-1] = mAction.size() - start;
			}
			break;
#endif

		case 0xddef486b /* "loop" */:
			{
				unsigned int name = Hash(child->Attribute("name"));
				float from = 0.0f;
				child->QueryFloatAttribute("from", &from);
				float to = 0.0f;
				child->QueryFloatAttribute("to", &to);
				float by = from < to ? 1.0f : -1.0f;
				child->QueryFloatAttribute("by", &by);

				if ((to - from) * by <= 0)
				{
					DebugPrint("loop name=\"%s\" from=\"%f\" to=\"%f\" by=\"%f\" would never terminate\n");
					break;
				}

				Expression::Append(mAction, Expression::Loop, name);
				Expression::Append(mAction, from, to, by);

				mAction.push_back(0);
				int start = mAction.size();
				ConfigureAction(child, aId);
				mAction[start-1] = mAction.size() - start;
			}
			break;
		}
	}

	return true;
}


// WAVE

// wave template constructor
WaveTemplate::WaveTemplate(void)
: mPreDelay(0.0f)
, mTrack(0)
, mPostDelay(0.0f)
{
}

// wave template configure
bool WaveTemplate::Configure(const TiXmlElement *element, unsigned int aId, unsigned int aWaveId)
{
	element->QueryFloatAttribute("predelay", &mPreDelay);
	element->QueryFloatAttribute("postdelay", &mPostDelay);

	// process child elements
	for (const TiXmlElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
	{
		const char *label = child->Value();
		switch (Hash(label))
		{
		case 0xc4642eff /* "action" */:
			ConfigureAction(child, aId);
			break;
		}
	}
	return true;
}


// WAVE SEQUENCE

// wavesequence template constructor
WaveSequenceTemplate::WaveSequenceTemplate(void)
{
}

// wavesequence template destructor
WaveSequenceTemplate::~WaveSequenceTemplate(void)
{
}

// wavesequence template configure
bool WaveSequenceTemplate::Configure(const TiXmlElement *element, unsigned int aId)
{
	element->QueryFloatAttribute("predelay", &mPreDelay);
	element->QueryFloatAttribute("postdelay", &mPostDelay);

	// process child elements
	for (const TiXmlElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
	{
		const char *label = child->Value();
		switch (Hash(label))
		{
		case 0xa9f017d4 /* "wave" */:
			if (const char *name = child->Attribute("name"))
			{
				unsigned int aWaveId = Hash(name);
				WaveTemplate &wave = mWaves.Open(aWaveId);
				wave.Configure(child, aId, aWaveId);
				mWaves.Close(aWaveId);
			}
			break;

		case 0xfe9c11ec /* "restart" */:
			if (const char *name = child->Attribute("name"))
			{
				mRestart = Hash(name);
			}
			break;
		}
	}

	return true;
}


// wavesequence default constructor
WaveSequence::WaveSequence(void)
: Updatable(0)
, mTrack(0)
, mTimer(0)
{
}

// wavesequence instantiation constructor
WaveSequence::WaveSequence(const WaveSequenceTemplate &aTemplate, unsigned int aId)
: Updatable(aId)
, mEntryIndex(0)
, mWaveIndex(-1)
, mTrack(0)
, mTimer(0)
{
	SetAction(Action(this, &WaveSequence::SequenceEnter));
}

// wavesequence destructor
WaveSequence::~WaveSequence(void)
{
	// remove listeners
}

void WaveSequence::SequenceEnter(float aStep)
{
	DebugPrint("%08x sequence enter\n", mId);

	mWaveIndex = 0;

	Deactivate();
	SetAction(Action(this, &WaveSequence::SequenceStartPreDelay));
	Activate();
}

void WaveSequence::SequenceStartPreDelay(float aStep)
{
	DebugPrint("%08x sequence start pre-delay\n", mId);

	const WaveSequenceTemplate &wavesequence = Database::wavesequencetemplate.Get(mId);

	mTimer -= wavesequence.mPreDelay;

	Deactivate();
	SetAction(Action(this, &WaveSequence::SequencePreDelay));
	Activate();
}

void WaveSequence::SequencePreDelay(float aStep)
{
	DebugPrint("%08x sequence pre-delay\n", mId);

	mTimer += aStep;
	if (mTimer < 0.0f)
		return;

	const WaveSequenceTemplate &wavesequence = Database::wavesequencetemplate.Get(mId);

	mTimer = 0.0f;
	Database::Typed<WaveTemplate>::Iterator waveitor(&wavesequence.mWaves, mWaveIndex);
	if (waveitor.IsValid())
		SetAction(Action(this, &WaveSequence::WaveEnter));
	else
		SetAction(Action(this, &WaveSequence::SequencePostDelay));
}

void WaveSequence::WaveEnter(float aStep)
{
	DebugPrint("%08x wave %d enter\n", mId, mWaveIndex);

	mEntryIndex = 0;

	SetAction(Action(this, &WaveSequence::WaveStartPreDelay));
}

void WaveSequence::WaveStartPreDelay(float aStep)
{
	DebugPrint("%08x wave %d start pre-delay\n", mId, mWaveIndex);

	const WaveSequenceTemplate &wavesequence = Database::wavesequencetemplate.Get(mId);

	Database::Typed<WaveTemplate>::Iterator waveitor(&wavesequence.mWaves, mWaveIndex);
	assert(waveitor.IsValid());

	mTimer -= waveitor.GetValue().mPreDelay;

	SetAction(Action(this, &WaveSequence::WavePreDelay));
}

void WaveSequence::WavePreDelay(float aStep)
{
	DebugPrint("%08x wave %d pre-delay\n", mId, mWaveIndex);

	mTimer += aStep;
	if (mTimer < 0.0f)
		return;

	const WaveSequenceTemplate &wavesequence = Database::wavesequencetemplate.Get(mId);

	Database::Typed<WaveTemplate>::Iterator waveitor(&wavesequence.mWaves, mWaveIndex);
	assert(waveitor.IsValid());

	if (waveitor.GetValue().mAction.size())
		SetAction(Action(this, &WaveSequence::WaveSpawn));
	else
		SetAction(Action(this, &WaveSequence::WaveStartPostDelay));
}

void WaveSequence::WaveSpawn(float aStep)
{
	// advance the timer
	mTimer += aStep;

	// if still waiting...
	if (mTimer < -0.001f)
	{
		// done
		return;
	}

	DebugPrint("%08x wave %d spawn\n", mId, mWaveIndex);

	const WaveSequenceTemplate &wavesequence = Database::wavesequencetemplate.Get(mId);

	Database::Typed<WaveTemplate>::Iterator waveitor(&wavesequence.mWaves, mWaveIndex);
	assert(waveitor.IsValid());

	// get template data
	const WaveTemplate &wave = waveitor.GetValue();

	// create an entity context
	EntityContext context(&wave.mAction.front(), wave.mAction.size(), mTimer, mId);
	context.mStream += mEntryIndex;

	// evaluate actions
	while (context.mParam > -0.001f && context.mStream < context.mEnd)
		Expression::Evaluate<void>(context);

	// read updated context
	mEntryIndex = context.mStream - context.mBegin;
	mTimer = context.mParam;

	// if the action is completed or out of track slots...
	if (context.mStream >= context.mEnd || wave.mTrack && mTrack >= wave.mTrack)
	{
		// update fire timer
		mTimer += mTimer;

		if (mTimer > -aStep)
		{
			// advance to next event
			aStep = -mTimer;
			mTimer = 0.0f;
		}

		SetAction(Action(this, &WaveSequence::WaveTrack));
	}
}

void WaveSequence::WaveTrack(float aStep)
{
	DebugPrint("%08x wave %d track\n", mId, mWaveIndex);
	if (mTrack > 0)
		return;

	SetAction(Action(this, &WaveSequence::WaveStartPostDelay));
}

void WaveSequence::WaveStartPostDelay(float aStep)
{
	DebugPrint("%08x wave %d start post-delay\n", mId, mWaveIndex);

	const WaveSequenceTemplate &wavesequence = Database::wavesequencetemplate.Get(mId);

	Database::Typed<WaveTemplate>::Iterator waveitor(&wavesequence.mWaves, mWaveIndex);
	assert(waveitor.IsValid());

	mTimer -= waveitor.GetValue().mPostDelay;

	Deactivate();
	SetAction(Action(this, &WaveSequence::WavePostDelay));
	Activate();
}

void WaveSequence::WavePostDelay(float aStep)
{
	DebugPrint("%08x wave %d post-delay\n", mId, mWaveIndex);

	mTimer += aStep;
	if (mTimer < 0.0f)
		return;

	SetAction(Action(this, &WaveSequence::WaveExit));
}

void WaveSequence::WaveExit(float aStep)
{
	DebugPrint("%08x wave %d exit\n", mId, mWaveIndex);

	++mWaveIndex;

	const WaveSequenceTemplate &wavesequence = Database::wavesequencetemplate.Get(mId);

	Deactivate();
	Database::Typed<WaveTemplate>::Iterator waveitor(&wavesequence.mWaves, mWaveIndex);
	if (waveitor.IsValid())
		SetAction(Action(this, &WaveSequence::WaveEnter));
	else if (wavesequence.mRestart)
		SetAction(Action(this, &WaveSequence::WaveRestart));
	else
		SetAction(Action(this, &WaveSequence::SequenceStartPostDelay));
	Activate();
}

void WaveSequence::WaveRestart(float aStep)
{
	const WaveSequenceTemplate &wavesequence = Database::wavesequencetemplate.Get(mId);
	for (Database::Typed<WaveTemplate>::Iterator itor(&wavesequence.mWaves); itor.IsValid(); ++itor)
	{
		if (itor.GetKey() == wavesequence.mRestart)
		{
			mWaveIndex = itor.GetSlot();
			SetAction(Action(this, &WaveSequence::WaveEnter));
			return;
		}
	}

	SetAction(Action(this, &WaveSequence::SequenceStartPostDelay));
}

void WaveSequence::SequenceStartPostDelay(float aStep)
{
	DebugPrint("%08x sequence start post-delay\n", mId);

	const WaveSequenceTemplate &wavesequence = Database::wavesequencetemplate.Get(mId);

	mTimer -= wavesequence.mPostDelay;

	SetAction(Action(this, &WaveSequence::SequencePostDelay));
}

void WaveSequence::SequencePostDelay(float aStep)
{
	DebugPrint("%08x sequence post-delay\n", mId);

	mTimer += aStep;
	if (mTimer < 0.0f)
		return;

	SetAction(Action(this, &WaveSequence::SequenceExit));
}

void WaveSequence::SequenceExit(float aStep)
{
	DebugPrint("%08x sequence exit\n", mId);

	Deactivate();
}

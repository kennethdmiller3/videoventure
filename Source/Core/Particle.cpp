#include "StdAfx.h"
#include "Entity.h"
#include "Updatable.h"


typedef bool ParticleTemplate;

class Particle : public Updatable
{
public:
#ifdef USE_POOL_ALLOCATOR
	// allocation
	void *operator new(size_t aSize);
	void operator delete(void *aPtr);
#endif

	Particle(const ParticleTemplate &aTemplate, unsigned int aId)
		: Updatable(aId)
	{
		SetAction(Action(this, &Particle::Update));
		Activate();
	}

	void Update(float aStep)
	{
		if (Entity *entity = Database::entity.Get(mId))
		{
			entity->Step();
			entity->SetPosition(entity->GetPosition() + aStep * entity->GetVelocity());
			entity->SetAngle(entity->GetAngle() + aStep * entity->GetOmega());
		}
	}
};


#ifdef USE_POOL_ALLOCATOR

// particle pool
static MemoryPool sPool(sizeof(Particle));
void *Particle::operator new(size_t aSize)
{
	return sPool.Alloc();
}
void Particle::operator delete(void *aPtr)
{
	sPool.Free(aPtr);
}
#endif


namespace Database
{
	Typed<ParticleTemplate> particletemplate(0x9e95955d /* "particletemplate" */);
	Typed<Particle *> particle(0x8a8743bf /* "particle" */);

	namespace Loader
	{
		static void ParticleConfigure(unsigned int aId, const tinyxml2::XMLElement *element)
		{
			ParticleTemplate &particle = Database::particletemplate.Open(aId);
			particle = true;
			Database::particletemplate.Close(aId);
		}
		Configure particleconfigure(0x8a8743bf /* "particle" */, ParticleConfigure);
	}

	namespace Initializer
	{
		static void ParticleActivate(unsigned int aId)
		{
			const ParticleTemplate &particletemplate = Database::particletemplate.Get(aId);
			Particle *particle = new Particle(particletemplate, aId);
			Database::particle.Put(aId, particle);
		}
		Activate particleactivate(0x9e95955d /* "particletemplate" */, ParticleActivate);

		static void ParticleDeactivate(unsigned int aId)
		{
			if (Particle *particle = Database::particle.Get(aId))
			{
				delete particle;
				Database::particle.Delete(aId);
			}
		}
		Deactivate particledeactivate(0x9e95955d /* "particletemplate" */, ParticleDeactivate);
	}
}

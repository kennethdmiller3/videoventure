#pragma once

#include "Database.h"

class DamagableTemplate
{
public:
	float mHealth;
	unsigned int mSpawnOnDeath;

public:
	DamagableTemplate(void);
	~DamagableTemplate(void);

	bool Configure(TiXmlElement *element);
};

class Damagable
{
protected:
	unsigned int id;

	float mHealth;

public:
	class Listener
	{
	public:
		virtual void Damage(unsigned int aSourceId, float aDamage) = 0;
	};

	// listeners
	typedef stdext::hash_map<unsigned int, Listener *> ListenerMap;
	ListenerMap listeners;

public:
	Damagable(void);
	Damagable(const DamagableTemplate &aTemplate, unsigned int aId);
	virtual ~Damagable(void);

	void Damage(unsigned int aSourceId, float aDamage);

	float GetHealth(void)
	{
		return mHealth;
	}

	bool IsAlive(void)
	{
		return mHealth > 0.0f;
	}
};

namespace Database
{
	extern Typed<DamagableTemplate> damagabletemplate;
	extern Typed<Damagable *> damagable;
}

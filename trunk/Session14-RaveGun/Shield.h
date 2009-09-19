class ShieldTemplate
{
public:
	// ammo
	unsigned int mType;

	// base cost
	float mBase;

	// damage scale
	float mScale;

	// damage limit
	float mLimit;

	// cost per damage
	float mCost;

	// invulnerability time
	float mInvulnerable;

	// spawn
	unsigned int mSpawn;

public:
	ShieldTemplate(void);
	~ShieldTemplate(void);

	// configure
	bool Configure(const TiXmlElement *element);
};

class Shield
{
protected:
	// instance identifier
	unsigned int mId;

	// ammo pool
	unsigned int mAmmo;

	// last trigger time
	unsigned int mTurn;

public:
#ifdef USE_POOL_ALLOCATOR
	// allocation
	void *operator new(size_t aSize);
	void operator delete(void *aPtr);
#endif

	Shield(void);
	Shield(const ShieldTemplate &aTemplate, unsigned int aId);
	virtual ~Shield(void);

	// resource change listener
	void Change(unsigned int aId, unsigned int aSubId, unsigned int aSourceId, float aValue);

	// damage listener
	void Damage(unsigned int aId, unsigned int aSourceId, float aDamage);
};

namespace Database
{
	extern Typed<ShieldTemplate> weapontemplate;
	extern Typed<Shield *> weapon;
}

class ShieldTemplate
{
public:
	// ammo
	unsigned int mType;
	float mCost;

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

#pragma once

#include "Database.h"
#include "Signal.h"

const int COLLISION_LAYERS = 32;

// forward declarations
struct cpSpace;
struct cpBody;
struct cpShape;

struct GAME_API CollidableFilter
{
	CollidableFilter()
		: mGroup(0U)
		, mCategories(1U)
		, mMask(~0U)
	{
	}
	CollidableFilter(uintptr_t aGroup, unsigned int aCategories, unsigned int aMask)
		: mGroup(aGroup)
		, mCategories(aCategories)
		, mMask(aMask)
	{
	}

	uintptr_t mGroup;
	unsigned int mCategories;
	unsigned int mMask;
};

struct CollidableShapeDef
{
	CollidableShapeDef()
		: mIsSensor(false)
		, mElasticity(0.0f)
		, mFriction(0.2f)
		, mSurfaceVelocity(0.0f, 0.0f)
		, mDensity(0.0f)
		, mFilter()
	{
	}

	bool mIsSensor;
	float mElasticity;
	float mFriction;
	Vector2 mSurfaceVelocity;
	float mDensity;
	CollidableFilter mFilter;
};

struct CollidableCircleDef : public CollidableShapeDef
{
	CollidableCircleDef()
		: CollidableShapeDef()
		, mRadius(0.0f)
		, mOffset(0.0f, 0.0f)
	{
	}

	float mRadius;
	Vector2 mOffset;
};

struct CollidablePolygonDef : public CollidableShapeDef
{
	CollidablePolygonDef()
		: CollidableShapeDef()
		, mVertices()
		, mOffset(0.0f, 0.0f)
	{
	}

	std::vector<Vector2> mVertices;
	Vector2 mOffset;
};

class CollidableEdgeDef : public CollidableShapeDef
{
public:
	CollidableEdgeDef()
		: CollidableShapeDef()
		, mA(0.0f, 0.0f)
		, mB(0.0f, 0.0f)
		, mRadius(0.0f)
	{
	}

public:
	Vector2 mA;
	Vector2 mB;
	float mRadius;
};

class CollidableChainDef : public CollidableShapeDef
{
public:
	CollidableChainDef()
		: CollidableShapeDef()
		, mVertices()
		, mRadius(0.0f)
		, mLoop(false)
	{
	}

public:
	std::vector<Vector2> mVertices;
	float mRadius;
	bool mLoop;
};

typedef cpShape CollidableShape;

extern GAME_API void ConfigureFilterData(CollidableFilter &aFilter, const tinyxml2::XMLElement *element);

class CollidableBodyDef
{
public:
	CollidableBodyDef()
		: mType(kType_Auto)
		, mMass(0.0f)
		, mMoment(0.0f)
		, mTransform(Transform2::Identity())
		, mVelocity(Transform2::Identity())
		, mLinearDamping(0.0f)
		, mAngularDamping(0.0f)
		, mFixedRotation(false)
	{
	}

public:
	// body definition
	enum Type
	{
		kType_Auto = -1,
		kType_Static,
		kType_Kinematic,
		kType_Dynamic
	};
	Type mType;
	float mMass;
	float mMoment;
	Transform2 mTransform;
	Transform2 mVelocity;
	float mLinearDamping;
	float mAngularDamping;
	bool mFixedRotation;
};

typedef cpBody CollidableBody;

class GAME_API CollidableTemplate
{
public:
	// identifier
	unsigned int mId;

	// body definition
	CollidableBodyDef mBodyDef;

public:
	CollidableTemplate(void);
	CollidableTemplate(const CollidableTemplate &aTemplate);
	~CollidableTemplate(void);

	// configure
	bool ConfigureShapeItem(const tinyxml2::XMLElement *element, CollidableShapeDef &shape);
	bool ConfigureShape(const tinyxml2::XMLElement *element, CollidableShapeDef &shape);
	bool ConfigureCircle(const tinyxml2::XMLElement *element, CollidableCircleDef &shape);
	bool ConfigureBox(const tinyxml2::XMLElement *element, CollidablePolygonDef &shape);
	bool ConfigurePolyItem(const tinyxml2::XMLElement *element, CollidablePolygonDef &shape);
	bool ConfigurePoly(const tinyxml2::XMLElement *element, CollidablePolygonDef &shape);
	bool ConfigureEdge(const tinyxml2::XMLElement *element, CollidableEdgeDef &shape);
	bool ConfigureChain(const tinyxml2::XMLElement *element, CollidableChainDef &shape);
	bool ConfigureBodyItem(const tinyxml2::XMLElement *element, CollidableBodyDef &body, unsigned int id);
	bool ConfigureBody(const tinyxml2::XMLElement *element, CollidableBodyDef &body, unsigned int id);
	bool Configure(const tinyxml2::XMLElement *element, unsigned int id);
};

namespace Collidable
{
	typedef Signal<void (unsigned int id1, unsigned int id2, float t, const Vector2 &contact, const Vector2 &normal)> ContactSignal;
	typedef Signal<void (unsigned int id1, unsigned int id2, float t)> SeparateSignal;

	// initialize the physics world
	void WorldInit(float aMinX, float aMinY, float aMaxX, float aMaxY, bool aWall);

	// clean up the physics world
	void WorldDone(void);

	// add an entity to the physics world
	GAME_API void AddToWorld(Database::Key aId);

	// remove an entity from the physics world
	GAME_API void RemoveFromWorld(Database::Key aId);

	// get the physics world
	GAME_API cpSpace *GetWorld(void);

	// get the world boundary
	GAME_API const AlignedBox2 &GetBoundary(void);

	// default filter
	inline const CollidableFilter &GetDefaultFilter(void)
	{
		static CollidableFilter filter;
		return filter;
	}

	// check filtering
	inline bool CheckFilter(const CollidableFilter &aFilter1, const CollidableFilter &aFilter2)
	{
		if (aFilter1.mGroup != 0 && aFilter1.mGroup == aFilter2.mGroup)
			return false;
		if ((aFilter1.mCategories & aFilter2.mMask) == 0)
			return false;
		if ((aFilter2.mCategories & aFilter1.mMask) == 0)
			return false;
		return true;
	}

	// test segment for intersection with world shapes
	GAME_API unsigned int TestSegment(const Vector2 &aStart, const Vector2 &aEnd,
									  const CollidableFilter &aFilter, unsigned int aId,
									  float &aLambda, Vector2 &aNormal, CollidableShape *&aShape);

	// query all shapes within an axis-aligned bounding box
	typedef fastdelegate::FastDelegate<void (CollidableShape *aShape)> QueryBoxDelegate;
	GAME_API void QueryBox(const AlignedBox2 &aBox, const CollidableFilter &aFilter, QueryBoxDelegate aDelegate);

	// query all shapes within radius of a point
	typedef fastdelegate::FastDelegate<void(CollidableShape *aShape, float aRange, const Vector2 &aPoint)> QueryRadiusDelegate;
	GAME_API void QueryRadius(const Vector2 &aCenter, float aRadius, const CollidableFilter &aFilter, const QueryRadiusDelegate aDelegate);

	// is a shape a sensor?
	GAME_API bool IsSensor(CollidableShape *aShape);

	// get the collision filter for a shape
	GAME_API CollidableFilter GetFilter(CollidableShape *aShape);

	// get the owner id of a shape
	GAME_API unsigned int GetId(CollidableShape *aShape);

	// get the center of a shape
	GAME_API Vector2 GetCenter(CollidableShape *aShape);

	// set the position of a body
	GAME_API void SetPosition(CollidableBody *aBody, const Vector2 &aPosition);

	// set the angle of a body
	GAME_API void SetAngle(CollidableBody *aBody, const float aAngle);

	// set the linear velocity of a body
	GAME_API void SetVelocity(CollidableBody *aBody, const Vector2 &aVelocity);

	// add linear velocity to a body
	GAME_API void AddVelocity(CollidableBody *aBody, const Vector2 &aDelta);

	// set the angular velocity of a body
	GAME_API void SetOmega(CollidableBody *aBody, const float aOmega);

	// add angular velocity to a body
	GAME_API void AddOmega(CollidableBody *aBody, const float aDelta);

	// apply an impulse to a body
	GAME_API void ApplyImpulse(CollidableBody *aBody, const Vector2 &aImpulse);

	// control
	void CollideAll(float aStep);
};

namespace Database
{
	extern GAME_API Typed<CollidableTemplate> collidabletemplate;
	extern GAME_API Typed<CollidableBody *> collidablebody;
	extern GAME_API Typed<Collidable::ContactSignal> collidablecontactadd;
	extern GAME_API Typed<Collidable::SeparateSignal> collidablecontactremove;
}

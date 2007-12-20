#include "stdafx.h"
#include "Database.h"
#include "Entity.h"
#include "Collidable.h"
#include "Renderable.h"
#include "Damagable.h"
#include "Bullet.h"
#include "Explosion.h"
#include "Spawner.h"
#include "Player.h"
#include "Gunner.h"
#include <new>

namespace Database
{
	Typed<unsigned int> parent("parent");

	Untyped::Untyped(unsigned int aId, size_t aStride)
		: mId(aId), mStride(aStride), mBits(8), mLimit(1<<mBits), mCount(0)
	{
		mMap = static_cast<size_t *>(malloc(mLimit * 2 * sizeof(size_t)));
		memset(mMap, EMPTY, mLimit * 2 * sizeof(size_t));

		mKey = static_cast<Key *>(malloc(mLimit * sizeof(Key)));
		memset(mKey, 0, mLimit * sizeof(Key));

		mPool = static_cast<void **>(malloc((mLimit >> SHIFT) * sizeof(void *)));
		memset(mPool, 0, (mLimit >> SHIFT) * sizeof(void *));
	}

	Untyped::~Untyped()
	{
		free(mMap);
		free(mKey);
		for (size_t slot = 0; slot < mLimit >> SHIFT; ++slot)
			free(mPool[slot]);
		free(mPool);
	}

	void Untyped::Clear(void)
	{
		memset(mMap, EMPTY, mLimit * 2 * sizeof(size_t));
		memset(mKey, 0, mLimit * sizeof(Key));
		for (size_t slot = 0; slot < mCount; ++slot)
			DeleteRecord(GetRecord(slot));
		mCount = 0;
	}

	void Untyped::Grow(void)
	{
		// resize
		++mBits;
		mLimit = 1<<mBits;

		// reallocate map
		free(mMap);
		mMap = static_cast<size_t *>(malloc(mLimit * 2 * sizeof(size_t)));
		memset(mMap, EMPTY, mLimit * 2 * sizeof(size_t));

		// reallocate keys
		mKey = static_cast<Key *>(realloc(mKey, mLimit * sizeof(size_t)));
		memset(mKey + (mLimit >> 1), 0, (mLimit >> 1) * sizeof(size_t));

		// reallocate pools
		mPool = static_cast<void **>(realloc(mPool, (mLimit >> SHIFT) * sizeof(size_t)));
		memset(mPool + (mLimit >> (SHIFT + 1)), 0, (mLimit >> (SHIFT + 1)) * sizeof(void *));

		// rebuild hash
		for (size_t record = 0; record < mCount; ++record)
		{
			// convert key to a hash map index
			// (HACK: assume key is already a hash)
			size_t key = mKey[record];
			size_t index = Index(key);

			// while the slot is not empty...
			size_t slot = mMap[index];
			while (slot != EMPTY)
			{
				// go to the next index
				index = Next(index);
				slot = mMap[index];
			}

			// insert the record key
			mMap[index] = record;
		}
	}

	void Untyped::Copy(const Untyped &aSource)
	{
		// clear existing pool
		Clear();

		// copy counts
		mBits = aSource.mBits;
		mLimit = aSource.mLimit;
		mCount = aSource.mCount;

		// copy map
		free(mMap);
		mMap = static_cast<size_t *>(malloc(mLimit * 2 * sizeof(size_t)));
		memcpy(mMap, aSource.mMap, mLimit * 2 * sizeof(size_t));

		// copy keys
		free(mKey);
		mKey = static_cast<Key *>(malloc(mLimit * sizeof(size_t)));
		memcpy(mKey, aSource.mKey, mLimit * sizeof(size_t));

		// copy pools
		free(mPool);
		mPool = static_cast<void **>(malloc((mLimit >> SHIFT) * sizeof(size_t)));
		memset(mPool, 0, (mLimit >> SHIFT) * sizeof(void *));
		for (size_t slot = 0; slot < mCount; ++slot)
		{
			if ((slot & ((1 << SHIFT) - 1)) == 0)
			{
				mPool[slot >> SHIFT] = malloc((1 << SHIFT) * mStride);
			}
			CreateRecord(GetRecord(slot), aSource.GetRecord(slot));
		}
	}

	const void *Untyped::Find(Key aKey) const
	{
		// convert key to a hash map index
		// (HACK: assume key is already a hash)
		size_t index = Probe(aKey);

		// if the slot is not empty...
		size_t slot = mMap[index];
		if (slot != EMPTY)
		{
			// return the record
			return GetRecord(slot);
		}

		// not found
		return NULL;
	}

	void Untyped::Put(Key aKey, const void *aValue)
	{
		// convert key to a hash map index
		// (HACK: assume key is already a hash)
		size_t index = Probe(aKey);

		// if the slot is not empty...
		size_t slot = mMap[index];
		if (slot != EMPTY)
		{
			// update the record
			UpdateRecord(GetRecord(slot), aValue);
			return;
		}

		// grow if the database is full
		if (mCount >= mLimit)
			Grow();

		// add a new record
		slot = mCount++;
		mMap[index] = slot;
		mKey[slot] = aKey;
		if (mPool[slot >> SHIFT] == NULL)
			mPool[slot >> SHIFT] = malloc(mStride << SHIFT);
		CreateRecord(GetRecord(slot), aValue);
	}

	void *Untyped::Open(Key aKey)
	{
		// convert key to a hash map index
		// (HACK: assume key is already a hash)
		size_t index = Probe(aKey);

		// if the slot is not empty...
		size_t slot = mMap[index];
		if (slot != EMPTY)
		{
			// return the record
			return GetRecord(slot);
		}

		// grow if the database is full
		if (mCount >= mLimit)
			Grow();

		// add a new record
		slot = mCount++;
		mMap[index] = slot;
		mKey[slot] = aKey;
		if (mPool[slot >> SHIFT] == NULL)
			mPool[slot >> SHIFT] = malloc(mStride << SHIFT);
		CreateRecord(GetRecord(slot));

		// return the record
		return GetRecord(slot);
	}

	void Untyped::Close(Key aKey)
	{
	}

	void Untyped::Delete(Key aKey)
	{
		// convert key to a hash map index
		// (HACK: assume key is already a hash)
		size_t index = Probe(aKey);

		// if the slot is empty...
		size_t slot = mMap[index];
		if (slot == EMPTY)
		{
			// not found
			return;
		}

		// update record count
		--mCount;

		// if not the last record...
		if (slot < mCount)
		{
			// move the last record into the slot
			Key key = mKey[slot] = mKey[mCount];
			CreateRecord(GetRecord(slot), GetRecord(mCount));

			// update the entry
			for (size_t keyindex = Index(key); mMap[keyindex] != EMPTY; keyindex = Next(keyindex))
			{
				if (mMap[keyindex] == mCount)
				{
					mMap[keyindex] = slot;
					break;
				}
			}
		}

		// delete the last record
		DeleteRecord(GetRecord(mCount));
		mKey[mCount] = 0;

		// for each entry in the cluster...
		size_t nextindex = index;
		while (1)
		{
			// get the next entry
			nextindex = Next(nextindex);
			size_t nextslot = mMap[nextindex];

			// stop upon reaching the end of the cluster
			if (nextslot == EMPTY)
				break;

			// if the entry is out of place, and there is a place for it...
			Key key = mKey[nextslot];
			size_t keyindex = Index(key);
			if ((nextindex > index && (keyindex <= index || keyindex > nextindex)) ||
				(nextindex < index && (keyindex <= index && keyindex > nextindex)))
			{
				// move the entry
				mMap[index] = mMap[nextindex];
				index = nextindex;
			}
		}

		// clear the empty slot
		mMap[index] = EMPTY;
	}


	// instantiate a template
	unsigned int Instantiate(unsigned int aTemplateId, float aAngle, Vector2 aPosition, Vector2 aVelocity)
	{
		// generate an instance identifier
		const unsigned int aInstanceTag = Entity::TakeId();
		const unsigned int aInstanceId = Hash(&aInstanceTag, sizeof(aInstanceTag), aTemplateId);

		// create a new entity
		Entity *entity = new Entity(aInstanceId);
		entity->SetTransform(aAngle, aPosition);
		entity->SetVelocity(aVelocity);
		entity->Step();
		Database::entity.Put(aInstanceId, entity);

		// inherit components from template
		Inherit(aInstanceId, aTemplateId);

		// activate the instance identifier
		Activate(aInstanceId);

		// return the instance identifier
		return aInstanceId;
	}

	// inherit from a template
	void Inherit(unsigned int aInstanceId, unsigned int aTemplateId)
	{
		// inherit collidable template
		const CollidableTemplate *collidabletemplate = Database::collidabletemplate.Find(aTemplateId);
		if (collidabletemplate)
		{
			Database::collidabletemplate.Put(aInstanceId, *collidabletemplate);
		}

		// inherit collidable bodies
		const Typed<b2BodyDef> *collidabletemplatebody = Database::collidabletemplatebody.Find(aTemplateId);
		if (collidabletemplatebody)
		{
			Database::collidabletemplatebody.Put(aInstanceId, *collidabletemplatebody);
		}

		// inherit renderable template
		const RenderableTemplate *renderabletemplate = Database::renderabletemplate.Find(aTemplateId);
		if (renderabletemplate)
		{
			Database::renderabletemplate.Put(aInstanceId, *renderabletemplate);
		}

		// inherit damagable template
		const DamagableTemplate *damagabletemplate = Database::damagabletemplate.Find(aTemplateId);
		if (damagabletemplate)
		{
			Database::damagabletemplate.Put(aInstanceId, *damagabletemplate);
		}

		// inherit bullet template
		const BulletTemplate *bullettemplate = Database::bullettemplate.Find(aTemplateId);
		if (bullettemplate)
		{
			Database::bullettemplate.Put(aInstanceId, *bullettemplate);
		}

		// inherit explosion template
		const ExplosionTemplate *explosiontemplate = Database::explosiontemplate.Find(aTemplateId);
		if (explosiontemplate)
		{
			Database::explosiontemplate.Put(aInstanceId, *explosiontemplate);
		}

		// inherit spawner template
		const SpawnerTemplate *spawnertemplate = Database::spawnertemplate.Find(aTemplateId);
		if (spawnertemplate)
		{
			Database::spawnertemplate.Put(aInstanceId, *spawnertemplate);
		}
	}

	// activate an identifier
	void Activate(unsigned int aId)
	{
		// initialize entity
		Entity *entity = Database::entity.Get(aId);

		// instantiate collidable template
		const CollidableTemplate *collidabletemplate = Database::collidabletemplate.Find(aId);
		if (collidabletemplate)
		{
			Collidable *collidable = new Collidable(*collidabletemplate, aId);
			Database::collidable.Put(aId, collidable);
			collidable->AddToWorld();
		}

		// instantiate renderable template
		const RenderableTemplate *renderabletemplate = Database::renderabletemplate.Find(aId);
		if (renderabletemplate)
		{
			Renderable *renderable = new Renderable(*renderabletemplate, aId);
			Database::renderable.Put(aId, renderable);
			renderable->Show();
		}

		// instantiate damagable template
		const DamagableTemplate *damagabletemplate = Database::damagabletemplate.Find(aId);
		if (damagabletemplate)
		{
			Damagable *damagable = new Damagable(*damagabletemplate, aId);
			Database::damagable.Put(aId, damagable);
		}

		// instantiate bullet template
		const BulletTemplate *bullettemplate = Database::bullettemplate.Find(aId);
		if (bullettemplate)
		{
			Bullet *bullet = new Bullet(*bullettemplate, aId);
			Database::bullet.Put(aId, bullet);
		}

		// instantiate explosion template
		const ExplosionTemplate *explosiontemplate = Database::explosiontemplate.Find(aId);
		if (explosiontemplate)
		{
			Explosion *explosion = new Explosion(*explosiontemplate, aId);
			Database::explosion.Put(aId, explosion);
		}

		// instantiate spawner template
		const SpawnerTemplate *spawnertemplate = Database::spawnertemplate.Find(aId);
		if (spawnertemplate)
		{
			Spawner *spawner = new Spawner(*spawnertemplate, aId);
			Database::spawner.Put(aId, spawner);
		}

		// initialize entity (HACK)
		entity->Init();
	}

	// deactivate an identifier
	void Deactivate(unsigned int aId)
	{
		// remove components
		if (Bullet *b = bullet.Get(aId))
		{
			delete b;
			bullet.Delete(aId);
		}
		if (Explosion *e = explosion.Get(aId))
		{
			delete e;
			explosion.Delete(aId);
		}
		if (Spawner *s = spawner.Get(aId))
		{
			delete s;
			spawner.Delete(aId);
		}
		if (Player *p = player.Get(aId))
		{
			delete p;
			player.Delete(aId);
		}
		if (Gunner *g = gunner.Get(aId))
		{
			delete g;
			gunner.Delete(aId);
		}
		if (Collidable *c = collidable.Get(aId))
		{
			delete c;
			collidable.Delete(aId);
		}
		if (Renderable *r = renderable.Get(aId))
		{
			delete r;
			renderable.Delete(aId);
		}
		if (Damagable *d = damagable.Get(aId))
		{
			delete d;
			damagable.Delete(aId);
		}
	}

	// deletion queue
	std::vector<unsigned int> deletequeue(64);

	// delete an identifier
	void Delete(unsigned int aId)
	{
		deletequeue.push_back(aId);
	}

	// destroy the identifier
	void Destroy(unsigned int aId)
	{
		// deactivate
		Deactivate(aId);

		// remove template components
		collidabletemplate.Delete(aId);
		collidabletemplatebody.Delete(aId);
		renderabletemplate.Delete(aId);
		damagabletemplate.Delete(aId);
		bullettemplate.Delete(aId);
		explosiontemplate.Delete(aId);
		spawnertemplate.Delete(aId);

		// remove the entity
		Database::entity.Delete(aId);
	}

	// update the database system
	void Update(void)
	{
		for (std::vector<unsigned int>::iterator itor = deletequeue.begin(); itor != deletequeue.end(); ++itor)
			Destroy(*itor);
		deletequeue.clear();
	}
}

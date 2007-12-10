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

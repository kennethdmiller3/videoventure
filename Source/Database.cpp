#include "stdafx.h"
#include "Database.h"
#include "Entity.h"
#include "Collidable.h"
#include "Renderable.h"
#include "Bullet.h"
#include "Explosion.h"
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
		// instantiate collidable template
		const CollidableTemplate *collidabletemplate = Database::collidabletemplate.Find(aTemplateId);
		if (collidabletemplate)
		{
			Database::collidabletemplate.Put(aInstanceId, *collidabletemplate);
		}

		// instantiate renderable template
		const RenderableTemplate *renderabletemplate = Database::renderabletemplate.Find(aTemplateId);
		if (renderabletemplate)
		{
			Database::renderabletemplate.Put(aInstanceId, *renderabletemplate);
		}

		// instantiate bullet template
		const BulletTemplate *bullettemplate = Database::bullettemplate.Find(aTemplateId);
		if (bullettemplate)
		{
			Database::bullettemplate.Put(aInstanceId, *bullettemplate);
		}

		// instantiate explosion template
		const ExplosionTemplate *explosiontemplate = Database::explosiontemplate.Find(aTemplateId);
		if (explosiontemplate)
		{
			Database::explosiontemplate.Put(aInstanceId, *explosiontemplate);
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

		// initialize gunner (HACK)
		Gunner *gunner = Database::gunner.Get(aId);
		if (gunner)
		{
			gunner->Init();
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
	}

	// delete an identifier
	void Delete(unsigned int aId)
	{
		// deactivate
		Deactivate(aId);

		// remove template components
		bullettemplate.Delete(aId);
		explosiontemplate.Delete(aId);
		collidabletemplate.Delete(aId);
		renderabletemplate.Delete(aId);

		// remove the entity
		Database::entity.Delete(aId);
	}
}

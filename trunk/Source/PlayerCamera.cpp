#include "StdAfx.h"
#include "PlayerCamera.h"
#include "Player.h"
#include "Entity.h"
#include "Ship.h"
#include "Sound.h"


extern float VIEW_AIM;
extern float VIEW_AIM_FILTER;

extern Vector2 camerapos[2];


namespace Database
{
	Typed<PlayerCameraTemplate> playercameratemplate(0x3c35f30d /* "playercameratemplate" */);
	Typed<PlayerCamera *> playercamera(0xdc77fdcf /* "playercamera" */);

	namespace Loader
	{
		class PlayerCameraLoader
		{
		public:
			PlayerCameraLoader()
			{
				AddConfigure(0xdc77fdcf /* "playercamera" */, Entry(this, &PlayerCameraLoader::Configure));
			}

			void Configure(unsigned int aId, const TiXmlElement *element)
			{
				// configure player camera
				element->QueryFloatAttribute("aimoffset", &VIEW_AIM);
				element->QueryFloatAttribute("aimfilter", &VIEW_AIM_FILTER);
			}
		}
		playercameraloader;
	}
}


PlayerCamera::PlayerCamera(unsigned int aPlayerId = 0)
	: Updatable(aPlayerId)
{
	trackid = 0;
	trackpos[0] = Vector2(0, 0);
	trackpos[1] = Vector2(0, 0);
	trackaim = Vector2(0, 0);

	Updatable::SetAction(Updatable::Action(this, &PlayerCamera::Update));
}

PlayerCamera::~PlayerCamera()
{
}

void PlayerCamera::Update(float aStep)
{
	// get the player
	Player *player = Database::player.Get(Updatable::mId);

	// get the attached entity identifier
	unsigned int id = player->mAttach;
	if (id)
	{
		// get the entity
		if (Entity *entity = Database::entity.Get(id))
		{
			// track player position
			trackpos[0] = trackpos[1];
			trackpos[1] = entity->GetPosition();

			// set listener position
			Sound::listenerpos = trackpos[1];

			// if applying view aim
			if (VIEW_AIM)
			{
				Vector2 trackdelta;
				if (Database::ship.Get(id))
					trackdelta = Vector2(input[Input::AIM_HORIZONTAL], input[Input::AIM_VERTICAL]) - trackaim;
				else
					trackdelta = -trackaim;
				if (trackdelta.LengthSq() > FLT_EPSILON)
					trackaim += VIEW_AIM_FILTER * sim_step * trackdelta;
				trackpos[1] += trackaim * VIEW_AIM;
			}

#if 0
			// apply boundary limits (HACK)
			const b2AABB &boundary = Collidable::GetBoundary();
			trackpos[1].x = Lerp(boundary.lowerBound.x + VIEW_SIZE * 0.5f, boundary.upperBound.x - VIEW_SIZE * 0.5f, (trackpos[1].x - boundary.lowerBound.x) / (boundary.upperBound.x - boundary.lowerBound.x));
			trackpos[1].y = Lerp(boundary.lowerBound.y + VIEW_SIZE * 0.5f, boundary.upperBound.y - VIEW_SIZE * 0.5f, (trackpos[1].y - boundary.lowerBound.y) / (boundary.upperBound.y - boundary.lowerBound.y));
#endif
		}
	}

	// if the tracked identifier changes...
	if (trackid != id)
	{
		// snap position
		trackid = id;
		trackpos[0] = trackpos[1];
		trackaim = Vector2(0, 0);
	}

	// set camera position
	camerapos[0] = trackpos[0];
	camerapos[1] = trackpos[1];

#ifdef TEST_PATHING
	Pathing(entity->GetPosition(), trackpos[1] + aimpos[1] * 120 * VIEW_SIZE / 320, 4.0f);
#endif
}

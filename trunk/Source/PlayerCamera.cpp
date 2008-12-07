#include "StdAfx.h"
#include "PlayerCamera.h"
#include "Player.h"
#include "Entity.h"
#include "Ship.h"
#include "Sound.h"


extern Vector2 camerapos[2];

static Vector2 trackoffset(0, 0);
static Vector2 scrollmin(-FLT_MAX, -FLT_MAX);
static Vector2 scrollmax(FLT_MAX, FLT_MAX);
static Vector2 scrollorigin(0.0f, 0.0f);
static Vector2 scrollscale(1.0f, 1.0f);

namespace Database
{
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
				for (const TiXmlElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
				{
					switch(Hash(child->Value()))
					{
					case 0x14c8d3ca /* "offset" */:
						child->QueryFloatAttribute("x", &trackoffset.x);
						child->QueryFloatAttribute("y", &trackoffset.y);
						break;

					case 0x383251f6 /* "aim" */:
						child->QueryFloatAttribute("scale", &VIEW_AIM);
						child->QueryFloatAttribute("filter", &VIEW_AIM_FILTER);
						break;

					case 0xd97f9a4f /* "origin" */:
						child->QueryFloatAttribute("x", &scrollorigin.x);
						child->QueryFloatAttribute("y", &scrollorigin.y);
						break;

					case 0x82971c71 /* "scale" */:
						child->QueryFloatAttribute("x", &scrollscale.x);
						child->QueryFloatAttribute("y", &scrollscale.y);
						break;

					case 0x54eacca7 /* "boundary" */:
						child->QueryFloatAttribute("xmin", &scrollmin.x);
						child->QueryFloatAttribute("ymin", &scrollmin.y);
						child->QueryFloatAttribute("xmax", &scrollmax.x);
						child->QueryFloatAttribute("ymax", &scrollmax.y);
						break;
					}
				}
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
			// track position
			trackpos[0] = trackpos[1];
			trackpos[1] = entity->GetTransform().Transform(trackoffset);

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

			// apply scale and boundary
			trackpos[1].x = Clamp(scrollorigin.x + scrollscale.x * (trackpos[1].x - scrollorigin.x), scrollmin.x, scrollmax.x);
			trackpos[1].y = Clamp(scrollorigin.y + scrollscale.y * (trackpos[1].y - scrollorigin.y), scrollmin.y, scrollmax.y);
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

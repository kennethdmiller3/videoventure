#include "StdAfx.h"
#include "PlayerCamera.h"
#include "Player.h"
#include "Entity.h"
#include "Ship.h"
#include "Sound.h"


extern Vector2 camerapos[2];

class PlayerCameraTemplate
{
public:
	Vector2 mTrackOffset;
	float mViewAim;
	float mViewAimFilter;
	Vector2 mScrollMin;
	Vector2 mScrollMax;
	Vector2 mScrollOrigin;
	Vector2 mScrollScale;

public:
	PlayerCameraTemplate(void);

	bool Configure(const tinyxml2::XMLElement *element, unsigned int aId);
};

PlayerCameraTemplate::PlayerCameraTemplate(void)
: mTrackOffset(0, 0)
, mViewAim(0.0f)
, mViewAimFilter(0.0f)
, mScrollMin(-FLT_MAX, -FLT_MAX)
, mScrollMax(FLT_MAX, FLT_MAX)
, mScrollOrigin(0.0f, 0.0f)
, mScrollScale(1.0f, 1.0f)
{
}

bool PlayerCameraTemplate::Configure(const tinyxml2::XMLElement *element, unsigned int aId)
{
	// configure player camera
	for (const tinyxml2::XMLElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
	{
		switch(Hash(child->Value()))
		{
		case 0x14c8d3ca /* "offset" */:
			child->QueryFloatAttribute("x", &mTrackOffset.x);
			child->QueryFloatAttribute("y", &mTrackOffset.y);
			break;

		case 0x383251f6 /* "aim" */:
			child->QueryFloatAttribute("scale", &mViewAim);
			child->QueryFloatAttribute("filter", &mViewAimFilter);
			break;

		case 0xd97f9a4f /* "origin" */:
			child->QueryFloatAttribute("x", &mScrollOrigin.x);
			child->QueryFloatAttribute("y", &mScrollOrigin.y);
			break;

		case 0x82971c71 /* "scale" */:
			child->QueryFloatAttribute("x", &mScrollScale.x);
			child->QueryFloatAttribute("y", &mScrollScale.y);
			break;

		case 0x54eacca7 /* "boundary" */:
			child->QueryFloatAttribute("xmin", &mScrollMin.x);
			child->QueryFloatAttribute("ymin", &mScrollMin.y);
			child->QueryFloatAttribute("xmax", &mScrollMax.x);
			child->QueryFloatAttribute("ymax", &mScrollMax.y);
			break;
		}
	}
	return true;
}

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

			void Configure(unsigned int aId, const tinyxml2::XMLElement *element)
			{
				PlayerCameraTemplate &playercamera = Database::playercameratemplate.Open(aId);
				playercamera.Configure(element, aId);
				Database::playercameratemplate.Close(aId);
			}
		}
		playercameraloader;
	}
}


PlayerCamera::PlayerCamera(unsigned int aPlayerId = 0)
	: Updatable(aPlayerId)
	, mTrackId(0)
	, mTrackPos0(0, 0)
	, mTrackPos1(0, 0)
	, mTrackAim(0, 0)
{
	Updatable::SetAction(Updatable::Action(this, &PlayerCamera::Update));
}

PlayerCamera::~PlayerCamera()
{
}

void PlayerCamera::Update(float aStep)
{
	// get the player
	Player *player = Database::player.Get(mId);

	// get the attached entity identifier
	unsigned int id = player->mAttach;
	if (id)
	{
		// get the entity
		if (Entity *entity = Database::entity.Get(id))
		{
			// get player camera template
			const PlayerCameraTemplate &playercamera = Database::playercameratemplate.Get(mId);

			// track position
			mTrackPos0 = mTrackPos1;
			mTrackPos1 = entity->GetTransform().Transform(playercamera.mTrackOffset);

			// if applying view aim
			if (playercamera.mViewAim)
			{
				Vector2 trackdelta;
				if (Database::ship.Get(id))
					trackdelta = Vector2(input[Input::AIM_HORIZONTAL], input[Input::AIM_VERTICAL]) - mTrackAim;
				else
					trackdelta = -mTrackAim;
				if (trackdelta.LengthSq() > FLT_EPSILON)
					mTrackAim += playercamera.mViewAimFilter * sim_step * trackdelta;
				mTrackPos1 += mTrackAim * playercamera.mViewAim;
			}

			// apply scale and boundary
			mTrackPos1 -= playercamera.mScrollOrigin;
			mTrackPos1 *= playercamera.mScrollScale;
			mTrackPos1 += playercamera.mScrollOrigin;
			mTrackPos1.x = Clamp(mTrackPos1.x, playercamera.mScrollMin.x, playercamera.mScrollMax.x);
			mTrackPos1.y = Clamp(mTrackPos1.y, playercamera.mScrollMin.y, playercamera.mScrollMax.y);
		}
	}

	// if the tracked identifier changes...
	if (mTrackId != id)
	{
		// snap position
		mTrackId = id;
		mTrackPos0 = mTrackPos1;
		mTrackAim = Vector2(0, 0);
	}

	// set camera position
	camerapos[0] = mTrackPos0;
	camerapos[1] = mTrackPos1;

	// set listener position
	Sound::Listener(mTrackPos1, (mTrackPos1 - mTrackPos0) / aStep);	// HACK

#ifdef TEST_PATHING
	Pathing(entity->GetPosition(), mTrackPos1 + aimpos[1] * 120 * VIEW_SIZE / 240, 4.0f);
#endif
}

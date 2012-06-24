#include "StdAfx.h"
#include "PlayerOverlayReticule.h"
#include "PlayerController.h"
#include "Player.h"


// reticule handle (HACK)
extern GLuint reticule_handle;


//
// PLAYER OVERLAY: RETICULE
//

// constructor
PlayerOverlayReticule::PlayerOverlayReticule(unsigned int aPlayerId = 0)
	: Updatable(aPlayerId)
	, Overlay(aPlayerId)
{
	// clear aim position
	aimpos[0] = aimpos[1] = Vector2(0, 0);

	Updatable::SetAction(Updatable::Action(this, &PlayerOverlayReticule::Update));
	Overlay::SetAction(Overlay::Action(this, &PlayerOverlayReticule::Render));
}

// destructor
PlayerOverlayReticule::~PlayerOverlayReticule()
{
}

// update
void PlayerOverlayReticule::Update(float aStep)
{
	aimpos[0] = aimpos[1];
	aimpos[1] = Vector2(input[Input::AIM_HORIZONTAL], input[Input::AIM_VERTICAL]);
}

// render
void PlayerOverlayReticule::Render(unsigned int aId, float aTime, const Transform2 &aTransform)
{
	// get the player
	Player *player = Database::player.Get(aId);

	// get the attached entity identifier
	unsigned int id = player->mAttach;

	// if tracking an active controller...
	Controller *controller = Database::controller.Get(id);
	if (controller)
	{
		// get player controller template
		const PlayerControllerTemplate &controllertemplate = Database::playercontrollertemplate.Get(id);

		// show reticule if using move or aim cursor
		if (controllertemplate.mAim == PlayerControllerTemplate::MOVECURSOR ||
			controllertemplate.mAim == PlayerControllerTemplate::AIMCURSOR)
		{
			// draw reticule
			float x = 320 - 240 * Lerp(aimpos[0].x, aimpos[1].x, sim_fraction);
			float y = 240 - 240 * Lerp(aimpos[0].y, aimpos[1].y, sim_fraction);

			glPushMatrix();
			glTranslatef(x, y, 0.0f);
			glCallList(reticule_handle);
			glPopMatrix();
		}
	}
}

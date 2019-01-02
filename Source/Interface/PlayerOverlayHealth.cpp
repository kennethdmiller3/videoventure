#include "StdAfx.h"
#include "PlayerOverlayHealth.h"
#include "Player.h"
#include "Damagable.h"


// health gauge
static const Rect<float> healthrect =
{
	8, 24, 128, 8
};
static const Color4 healthbackground(0.3f, 0.3f, 0.3f, 0.5f);
static const Color4 healthdrain(1.0f, 0.0f, 0.0f, 0.5f);
static const Color4 healthcolor[3][2] =
{
	{ Color4( 1.0f, 0.0f, 0.0f, 1.0f ), Color4( 1.0f, 1.0f, 1.0f, 1.0f ) },	// critical health
	{ Color4( 1.0f, 1.0f, 0.0f, 1.0f ), Color4( 1.0f, 1.0f, 0.3f, 1.0f ) },	// warning health
	{ Color4( 0.0f, 1.0f, 0.0f, 1.0f ), Color4( 0.2f, 1.0f, 0.2f, 1.0f ) },	// normal health
};

// drain values
static const float DRAIN_DELAY = 1.0f;
static const float DRAIN_RATE = 0.5f;

// flash values
static const float FLASH_RATE = 2.0f;


//
// PLAYER OVERLAY: HEALTH
//

// constructor
PlayerOverlayHealth::PlayerOverlayHealth(unsigned int aPlayerId = 0)
	: Overlay(aPlayerId)
	, fill(0)
	, drain(0)
	, draindelay(0)
	, flashcount(0)
	, pulsetimer(0)
{
	Overlay::SetAction(Overlay::Action(this, &PlayerOverlayHealth::Render));
}

// destructor
PlayerOverlayHealth::~PlayerOverlayHealth()
{
}

// render
void PlayerOverlayHealth::Render(unsigned int aId, float aTime, const Transform2 &aTransform)
{
	// get the player
	Player *player = Database::player.Get(aId);

	// get the attached entity identifier
	unsigned int id = player->mAttach;

	// draw player health (HACK)
	float health = 0.0f;
	float maxhealth = 0;
	Damagable *damagable = Database::damagable.Get(id);
	if (damagable)
	{
		// get health ratio
		const DamagableTemplate &damagabletemplate = Database::damagabletemplate.Get(id);
		if (damagabletemplate.mHealth > 0)
		{
			maxhealth = damagabletemplate.mHealth;
			health = damagable->GetHealth() / maxhealth;
		}
	}

	// if health is greater than the gauge fill...
	if (fill < health - FLT_EPSILON)
	{
		// raise the fill
		fill = health;
		if (drain < fill)
			drain = fill;
	}
	// else if health is lower than the gauge fill...
	else if (fill > health + FLT_EPSILON)
	{
		// add a flash
		if (flashcount == SDL_arraysize(flash))
			--flashcount;
		for (int i = flashcount; i > 0; --i)
			flash[i] = flash[i-1];
		Flash &flashinfo = flash[0];
		flashinfo.left = health;
		flashinfo.right = fill;
		flashinfo.fade = 1.0f;
		++flashcount;

		// lower the fill
		fill = health;

		// reset the drain delay
		draindelay = DRAIN_DELAY;
	}

	// update pulse
	pulsetimer += frame_time * (1.0f + (1.0f - health) * (1.0f - health) * 4.0f);
	while (pulsetimer >= 1.0f)
		pulsetimer -= 1.0f;
	float pulse = sinf(pulsetimer * float(M_PI));
	pulse *= pulse;
	pulse *= pulse;
	pulse *= pulse;

	// set color based on health and pulse

	int band = (health > 0.5f);
	float ratio = health * 2.0f - band;

	Color4 fillcolor;
	for (int i = 0; i < 4; i++)
		fillcolor[i] = Lerp(Lerp(healthcolor[band][0][i], healthcolor[band+1][0][i], ratio), Lerp(healthcolor[band][1][i], healthcolor[band+1][1][i], ratio), pulse);

	// begin drawing
	glBegin(GL_QUADS);

	// background
	glColor4fv(healthbackground);
	glVertex2f(healthrect.x, healthrect.y);
	glVertex2f(healthrect.x + healthrect.w, healthrect.y);
	glVertex2f(healthrect.x + healthrect.w, healthrect.y + healthrect.h);
	glVertex2f(healthrect.x, healthrect.y + healthrect.h);

	// drain
	if (fill < drain)
	{
		glColor4fv(healthdrain);
		glVertex2f(healthrect.x + healthrect.w * fill, healthrect.y);
		glVertex2f(healthrect.x + healthrect.w * drain, healthrect.y);
		glVertex2f(healthrect.x + healthrect.w * drain, healthrect.y + healthrect.h);
		glVertex2f(healthrect.x + healthrect.w * fill, healthrect.y + healthrect.h);
	}

	// flash
	for (int i = 0; i < flashcount; ++i)
	{
		Flash &flashinfo = flash[i];
		glColor4f(1.0f, 1.0f, 1.0f, flashinfo.fade);
		glVertex2f(healthrect.x + healthrect.w * flashinfo.left, healthrect.y - 2 * flashinfo.fade);
		glVertex2f(healthrect.x + healthrect.w * flashinfo.right, healthrect.y - 2 * flashinfo.fade);
		glVertex2f(healthrect.x + healthrect.w * flashinfo.right, healthrect.y + healthrect.h + 2 * flashinfo.fade);
		glVertex2f(healthrect.x + healthrect.w * flashinfo.left, healthrect.y + healthrect.h + 2 * flashinfo.fade);
	}

	// fill gauge
	glColor4fv(fillcolor);
	glVertex2f(healthrect.x, healthrect.y);
	glVertex2f(healthrect.x + healthrect.w * fill, healthrect.y);
	glVertex2f(healthrect.x + healthrect.w * fill, healthrect.y + healthrect.h);
	glVertex2f(healthrect.x, healthrect.y + healthrect.h);

	glEnd();

	// if the drain delay elapsed...
	draindelay -= frame_time;
	if (draindelay <= 0)
	{
		// update drain
		drain -= DRAIN_RATE * frame_time;
		if (drain < fill)
			drain = fill;
	}

	// count down flash timers
	for (int i = 0; i < flashcount; ++i)
	{
		Flash &flashinfo = flash[i];
		flashinfo.fade -= FLASH_RATE * frame_time;
		if (flashinfo.fade <= 0.0f)
		{
			flashcount = i;
			break;
		}
	}

	if (maxhealth > 1)
	{
		glBegin(GL_LINES);
		glColor4f(0.0f, 0.0f, 0.0f, 0.125f);

		// tick marks
		int ticks = int(maxhealth);
		for (int i = 1; i < ticks; ++i)
		{
			float x = healthrect.x + i * healthrect.w / maxhealth;
			glVertex2f(x, healthrect.y);
			glVertex2f(x, healthrect.y + healthrect.h);
		}

		glEnd();
	}
}

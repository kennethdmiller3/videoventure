#include "StdAfx.h"
#include "Player.h"
#include "PlayerHUD.h"
#include "PlayerController.h"
#include "Damagable.h"
#include "Drawlist.h"
#include "Resource.h"

extern bool wasreset;

// text display (HACK)
extern "C" GLuint OGLCONSOLE_glFontHandle;
extern "C" void OGLCONSOLE_DrawString(char *s, double x, double y, double w, double h, double z);
extern "C" void OGLCONSOLE_DrawCharacter(int c, double x, double y, double w, double h, double z);

// reticule handle (HACK)
extern GLuint reticule_handle;


// drain values
static const float DRAIN_DELAY = 1.0f;
static const float DRAIN_RATE = 0.5f;

// flash values
static const float FLASH_RATE = 2.0f;

template<typename T> struct Rect
{
	T x;
	T y;
	T w;
	T h;
};

// health gauge
static const Rect<float> healthrect =
{
	8, 8, 128, 8
};
static const Color4 healthcolor[3][2] =
{
	{ Color4( 1.0f, 0.0f, 0.0f, 1.0f ),  Color4( 1.0f, 1.0f, 1.0f, 1.0f ) },
	{ Color4( 1.0f, 1.0f, 0.0f, 0.75f ), Color4( 1.0f, 1.0f, 0.3f, 0.75f ) },
	{ Color4( 0.0f, 1.0f, 0.0f, 0.5f ),  Color4( 0.2f, 1.0f, 0.2f, 0.5f ) },
};


namespace Database
{
	Typed<PlayerHUD *> playerhud(0x8e522e29 /* "playerhud" */);
}


PlayerHUD::PlayerHUD(unsigned int aPlayerId = 0)
	: Updatable(aPlayerId)
	, Overlay(aPlayerId)
	, fill(0)
	, drain(0)
	, draindelay(0)
	, flashcount(0)
{
	// allocate score draw list
	score_handle = glGenLists(1);

	// allocate lives draw list
	lives_handle = glGenLists(1);

	Updatable::SetAction(Updatable::Action(this, &PlayerHUD::Update));
	Overlay::SetAction(Overlay::Action(this, &PlayerHUD::Render));
}

PlayerHUD::~PlayerHUD()
{
}

void PlayerHUD::Update(float aStep)
{
	aimpos[0] = aimpos[1];
	aimpos[1] = Vector2(input[Input::AIM_HORIZONTAL], input[Input::AIM_VERTICAL]);
}

void PlayerHUD::Render(unsigned int aId, float aTime, float aPosX, float aPosY, float aAngle)
{
	// get the player
	Player *player = Database::player.Get(aId);

	// get the attached entity identifier
	unsigned int id = player->mAttach;

	// draw player health (HACK)
	float health = 0.0f;
	Damagable *damagable = Database::damagable.Get(id);
	if (damagable)
	{
		// get health ratio
		const DamagableTemplate &damagabletemplate = Database::damagabletemplate.Get(id);
		if (damagabletemplate.mHealth > 0)
		{
			health = damagable->GetHealth() / damagabletemplate.mHealth;
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
	static float pulsetimer = 0.0f;
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
	glColor4f(0.3f, 0.3f, 0.3f, 0.5f);
	glVertex2f(healthrect.x, healthrect.y);
	glVertex2f(healthrect.x + healthrect.w, healthrect.y);
	glVertex2f(healthrect.x + healthrect.w, healthrect.y + healthrect.h);
	glVertex2f(healthrect.x, healthrect.y + healthrect.h);

	// drain
	glColor4f(1.0f, 0.0f, 0.0f, 0.5f);
	glVertex2f(healthrect.x + healthrect.w * fill, healthrect.y);
	glVertex2f(healthrect.x + healthrect.w * drain, healthrect.y);
	glVertex2f(healthrect.x + healthrect.w * drain, healthrect.y + healthrect.h);
	glVertex2f(healthrect.x + healthrect.w * fill, healthrect.y + healthrect.h);

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

	// get player score
	static int cur_score = -1;
	int new_score = player->mScore;

	// if the score has not changed...
	if (new_score == cur_score && !wasreset)
	{
		// call the existing draw list
		glCallList(score_handle);
	}
	else
	{
		// update score
		cur_score = new_score;

		// start a new draw list list
		glNewList(score_handle, GL_COMPILE_AND_EXECUTE);

		// draw player score (HACK)
		char score[9];
		sprintf(score, "%08d", new_score);
		bool leading = true;

		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, OGLCONSOLE_glFontHandle);
		glBegin(GL_QUADS);

		float x = 8;
		float y = 32;
		float z = 0;
		float w = 16;
		float h = -16;
		static const float textcolor[2][3] =
		{
			{ 0.4f, 0.5f, 1.0f },
			{ 0.3f, 0.3f, 0.3f }
		};

		for (char *s = score; *s != '\0'; ++s)
		{
			char c = *s;
			if (c != '0')
				leading = false;
			glColor3fv(textcolor[leading]);
			OGLCONSOLE_DrawCharacter(c, x, y, w, h, z);
			x += w;
		}

		glEnd();

		glDisable(GL_TEXTURE_2D);

		glEndList();
	}

	int cur_lives = -1;
	int new_lives = player->mLives;
	if (new_lives < INT_MAX)
	{
		// if the lives has not changed...
		if (new_lives == cur_lives && !wasreset)
		{
			// call the existing draw list
			glCallList(lives_handle);
		}
		else
		{
			// update lives
			cur_lives = new_lives;

			// start a new draw list list
			glNewList(lives_handle, GL_COMPILE_AND_EXECUTE);

			// draw the player ship
			glColor4f(0.4f, 0.5f, 1.0f, 1.0f);
			glPushMatrix();
			glTranslatef(healthrect.x + healthrect.w + 8, healthrect.y + 4, 0.0f);
			glScalef(-0.5, -0.5, 1);
			glCallList(Database::drawlist.Get(0xeec1dafa /* "playership" */));
			glPopMatrix();

			// draw remaining lives
			char lives[16];
			sprintf(lives, "x%d", cur_lives);

			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, OGLCONSOLE_glFontHandle);

			glColor4f(0.4f, 0.5f, 1.0f, 1.0f);
			glBegin(GL_QUADS);
			float w = 8;
			float h = -8;
			float x = healthrect.x + healthrect.w + 16;
			float y = healthrect.y + 4 - 0.5f * h;
			float z = 0;
			OGLCONSOLE_DrawString(lives, x, y, w, h, z);

			glEnd();

			glDisable(GL_TEXTURE_2D);

			glEndList();
		}
	}

	// get "special" ammo resource (HACK)

	int cur_ammo = -1;
	int new_ammo = INT_MAX;
	if (Resource *specialammo = Database::resource.Get(aId).Get(0xd940d530 /* "special" */))
	{
		new_ammo = xs_FloorToInt(specialammo->GetValue());
	}
	if (new_ammo < INT_MAX)
	{
		// if the ammo has not changed...
		if (new_ammo == cur_ammo && !wasreset)
		{
			// call the existing draw list
			glCallList(ammo_handle);
		}
		else
		{
			// update ammo
			cur_ammo = new_ammo;

			// start a new draw list list
			glNewList(ammo_handle, GL_COMPILE_AND_EXECUTE);

			// draw the special ammo
			glColor4f(0.4f, 0.5f, 1.0f, 1.0f);
			glPushMatrix();
			glTranslatef(healthrect.x + healthrect.w + 8, healthrect.y + 16, 0.0f);
			glScalef(4, 4, 1);
			glCallList(Database::drawlist.Get(0x8cdedbba /* "circle16" */));
			glPopMatrix();

			// draw remaining ammo
			char ammo[16];
			sprintf(ammo, "x%d", cur_ammo);

			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, OGLCONSOLE_glFontHandle);

			glColor4f(0.4f, 0.5f, 1.0f, 1.0f);

			glBegin(GL_QUADS);
			float w = 8;
			float h = -8;
			float x = healthrect.x + healthrect.w + 16;
			float y = healthrect.y + 16 - 0.5f * h;
			float z = 0;
			OGLCONSOLE_DrawString(ammo, x, y, w, h, z);

			glEnd();

			glDisable(GL_TEXTURE_2D);

			glEndList();
		}
	}

	static float gameovertimer = 0.0f;

	// if tracking an active controller...
	Controller *controller = Database::controller.Get(id);
	if (controller)
	{
		if (Database::playercontrollertemplate.Get(id).mAim == PlayerControllerTemplate::AIMWORLD)
		{
			// draw reticule
			float x = 320 - 240 * Lerp(aimpos[0].x, aimpos[1].x, sim_fraction);
			float y = 240 - 240 * Lerp(aimpos[0].y, aimpos[1].y, sim_fraction);

			glPushMatrix();
			glTranslatef(x, y, 0.0f);
			glCallList(reticule_handle);
			glPopMatrix();
		}

		gameovertimer = 0.0f;
	}
	else if (cur_lives <= 0 && player->mTimer <= 0.0f)
	{
		// display game over
		gameovertimer += frame_time;
		if (gameovertimer > 1.0f)
			gameovertimer = 1.0f;

		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, OGLCONSOLE_glFontHandle);


		glBegin(GL_QUADS);

		static char *text = "GAME OVER";
		const float w = Lerp<float>(96, 48, gameovertimer);
		const float h = Lerp<float>(-64, -32, gameovertimer);
		const float x = 320 - 0.5f * w * strlen(text);
		const float y = 240 - 0.5f * h;
		const float z = 0;
		const float a = gameovertimer * gameovertimer;

		glColor4f(0.1f, 0.1f, 0.1f, a);
		OGLCONSOLE_DrawString(text, x - 2, y - 2, w, h, z);
		OGLCONSOLE_DrawString(text, x    , y - 2, w, h, z);
		OGLCONSOLE_DrawString(text, x + 2, y - 2, w, h, z);
		OGLCONSOLE_DrawString(text, x - 2, y    , w, h, z);
		OGLCONSOLE_DrawString(text, x + 2, y    , w, h, z);
		OGLCONSOLE_DrawString(text, x - 2, y + 2, w, h, z);
		OGLCONSOLE_DrawString(text, x    , y + 2, w, h, z);
		OGLCONSOLE_DrawString(text, x + 2, y + 2, w, h, z);

		glColor4f(1.0f, 0.2f, 0.1f, a);
		OGLCONSOLE_DrawString(text, x, y, w, h, z);

		glEnd();

		glDisable(GL_TEXTURE_2D);
	}
}

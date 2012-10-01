#include "StdAfx.h"
#include "PlayerOverlayHealth.h"
#include "Player.h"
#include "Damagable.h"
#include "Render.h"
#include "MatrixStack.h"
#include "ShaderColor.h"


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

struct Vertex
{
	Vector3 pos;
#ifdef PLAYER_HEALTH_FLOAT_COLOR
	Color4 color;
#else
	unsigned int color;
#endif
};


unsigned int ToPacked(const Color4 &color)
{
	return
		(Clamp(xs_RoundToInt(color.r * 255), 0, 255)) |
		(Clamp(xs_RoundToInt(color.g * 255), 0, 255) << 8) |
		(Clamp(xs_RoundToInt(color.b * 255), 0, 255) << 16) |
		(Clamp(xs_RoundToInt(color.a * 255), 0, 255) << 24);
}

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

	Color4 fillcolor = Lerp(Lerp(healthcolor[band][0], healthcolor[band+1][0], ratio), Lerp(healthcolor[band][1], healthcolor[band+1][1], ratio), pulse);

	// use the color shader
	if (UseProgram(ShaderColor::gProgramId) || &GetBoundVertexBuffer() != &GetDynamicVertexBuffer() || ViewProjChanged())
	{
		// shader changed or switching back from non-dynamic geometry:
		// set model view projection matrix
		SetUniformMatrix4(ShaderColor::gUniformModelViewProj, ViewProjGet());
	}

	// set attribute formats
	SetAttribFormat(ShaderColor::gAttribPosition, 3, GL_FLOAT);
	SetAttribFormat(ShaderColor::gAttribColor, 4, GL_UNSIGNED_BYTE);

	// set work buffer format
	SetWorkFormat((1<<ShaderColor::gAttribPosition)|(1<<ShaderColor::gAttribColor));

	SetDrawMode(GL_TRIANGLES);

	int base = GetVertexCount();
	register Vertex * __restrict v;

	if (fill > 0.0f)
	{
		// fill gauge
#ifdef PLAYER_HEALTH_FLOAT_COLOR
		const Color4 &color = fillcolor;
#else
		unsigned int color = ToPacked(fillcolor);
#endif
		v = static_cast<Vertex *>(AllocVertices(4));
		v->pos = Vector3(healthrect.x, healthrect.y, 0);
		v->color = color;
		++v;
		v->pos = Vector3(healthrect.x + healthrect.w * fill, healthrect.y, 0);
		v->color = color;
		++v;
		v->pos = Vector3(healthrect.x + healthrect.w * fill, healthrect.y + healthrect.h, 0);
		v->color = color;
		++v;
		v->pos = Vector3(healthrect.x, healthrect.y + healthrect.h, 0);
		v->color = color;
		++v;
	}

	if (fill < 1.0f)
	{
		// background
#ifdef PLAYER_HEALTH_FLOAT_COLOR
		const Color4 &color = healthbackground;
#else
		unsigned int color = ToPacked(healthbackground);
#endif
		v = static_cast<Vertex *>(AllocVertices(4));
		v->pos = Vector3(healthrect.x + healthrect.w * fill, healthrect.y, 0);
		v->color = color;
		++v;
		v->pos = Vector3(healthrect.x + healthrect.w, healthrect.y, 0);
		v->color = color;
		++v;
		v->pos = Vector3(healthrect.x + healthrect.w, healthrect.y + healthrect.h, 0);
		v->color = color;
		++v;
		v->pos = Vector3(healthrect.x + healthrect.w * fill, healthrect.y + healthrect.h, 0);
		v->color = color;
		++v;
	}

	// drain
	if (fill < drain)
	{
#ifdef PLAYER_HEALTH_FLOAT_COLOR
		const Color4 &color = healthdrain;
#else
		unsigned int color = ToPacked(healthdrain);
#endif
		v = static_cast<Vertex *>(AllocVertices(4));
		v->pos = Vector3(healthrect.x + healthrect.w * fill, healthrect.y, 0);
		v->color = color;
		++v;
		v->pos = Vector3(healthrect.x + healthrect.w * drain, healthrect.y, 0);
		v->color = color;
		++v;
		v->pos = Vector3(healthrect.x + healthrect.w * drain, healthrect.y + healthrect.h, 0);
		v->color = color;
		++v;
		v->pos = Vector3(healthrect.x + healthrect.w * fill, healthrect.y + healthrect.h, 0);
		v->color = color;
		++v;
	}

	// flash
	for (int i = 0; i < flashcount; ++i)
	{
		Flash &flashinfo = flash[i];
#ifdef PLAYER_HEALTH_FLOAT_COLOR
		const Color4 color(1.0f, 1.0f, 1.0f, flashinfo.fade);
#else
		unsigned int color = Clamp(xs_RoundToInt(flashinfo.fade * 255), 0, 255) << 24 | 0x00FFFFFF;
#endif
		v = static_cast<Vertex *>(AllocVertices(4));
		v->pos = Vector3(healthrect.x + healthrect.w * flashinfo.left, healthrect.y - 2 * flashinfo.fade, 0);
		v->color = color;
		++v;
		v->pos = Vector3(healthrect.x + healthrect.w * flashinfo.right, healthrect.y - 2 * flashinfo.fade, 0);
		v->color = color;
		++v;
		v->pos = Vector3(healthrect.x + healthrect.w * flashinfo.right, healthrect.y + healthrect.h + 2 * flashinfo.fade, 0);
		v->color = color;
		++v;
		v->pos = Vector3(healthrect.x + healthrect.w * flashinfo.left, healthrect.y + healthrect.h + 2 * flashinfo.fade, 0);
		v->color = color;
		++v;
	}

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
		// tick marks
		int ticks = xs_FloorToInt(maxhealth);
#ifdef PLAYER_HEALTH_FLOAT_COLOR
		const Color4 color(0.0f, 0.0f, 0.0f, 0.125f);
#else
		unsigned int color = 0x1F000000;
#endif
		v = static_cast<Vertex *>(AllocVertices((ticks - 1) * 4));

		for (int i = 1; i < ticks; ++i)
		{
			float x = healthrect.x + i * healthrect.w / maxhealth;
			v->pos = Vector3(x, healthrect.y, 0);
			v->color = color;
			++v;
			v->pos = Vector3(x + 1, healthrect.y, 0);
			v->color = color;
			++v;
			v->pos = Vector3(x + 1, healthrect.y + healthrect.h, 0);
			v->color = color;
			++v;
			v->pos = Vector3(x, healthrect.y + healthrect.h, 0);
			v->color = color;
			++v;
		}
	}

	// indices
	IndexQuads(base, GetVertexCount() - base);
}

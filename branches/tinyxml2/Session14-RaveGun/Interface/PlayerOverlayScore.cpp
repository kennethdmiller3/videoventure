#include "StdAfx.h"
#include "PlayerOverlayScore.h"
#include "Player.h"


// text display (HACK)
extern "C" GLuint OGLCONSOLE_glFontHandle;
extern "C" void OGLCONSOLE_DrawString(char *s, double x, double y, double w, double h, double z);
extern "C" void OGLCONSOLE_DrawCharacter(int c, double x, double y, double w, double h, double z);

// score properties
static const Rect<float> scorerect =
{
	8, 8, 16, 16
};
static const Color4 scorecolor[2] =
{
	Color4(0.4f, 0.5f, 1.0f, 1.0f),
	Color4(0.3f, 0.3f, 0.3f, 1.0f)
};


//
// PLAYER OVERLAY: SCORE
//

// constructor
PlayerOverlayScore::PlayerOverlayScore(unsigned int aPlayerId = 0)
	: Overlay(aPlayerId)
	, cur_score(-1)
{
	// allocate score draw list
	score_handle = glGenLists(1);

	Overlay::SetAction(Overlay::Action(this, &PlayerOverlayScore::Render));
}

// destructor
PlayerOverlayScore::~PlayerOverlayScore()
{
	// free score draw list
	glDeleteLists(score_handle, 1);
}

// render
void PlayerOverlayScore::Render(unsigned int aId, float aTime, const Transform2 &aTransform)
{
	// get the player
	Player *player = Database::player.Get(aId);

	// get player score
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

		for (char *s = score; *s != '\0'; ++s)
		{
			char c = *s;
			if (c != '0')
				leading = false;
			glColor4fv(scorecolor[leading]);
			OGLCONSOLE_DrawCharacter(c,
				scorerect.x + scorerect.w * (s - score), scorerect.y + scorerect.h,
				scorerect.w, -scorerect.h, 0);
		}

		glEnd();

		glDisable(GL_TEXTURE_2D);

		glEndList();
	}
}

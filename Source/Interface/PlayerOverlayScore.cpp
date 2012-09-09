#include "StdAfx.h"
#include "PlayerOverlayScore.h"
#include "Player.h"
#include "Font.h"


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
	if (glIsList(score_handle))
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
	if (new_score == cur_score && glIsList(score_handle))
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

		FontDrawBegin(sDefaultFontHandle);

		for (char *s = score; *s != '\0'; ++s)
		{
			char c = *s;
			if (c != '0')
				leading = false;
			FontDrawColor(scorecolor[leading]);
			FontDrawCharacter(c,
				scorerect.x + scorerect.w * (s - score), scorerect.y + scorerect.h,
				scorerect.w, -scorerect.h, 0);
		}

		FontDrawEnd();

		glEndList();
	}
}

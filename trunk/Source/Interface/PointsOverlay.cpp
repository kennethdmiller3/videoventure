#include "StdAfx.h"
#include "PointsOverlay.h"


extern "C" GLuint OGLCONSOLE_glFontHandle;
extern "C" void OGLCONSOLE_DrawString(char *s, double x, double y, double w, double h, double z);


namespace Database
{
	Typed<PointsOverlay *> pointsoverlay(0x325ede2e /* "pointsoverlay" */);
}


// default constructor
PointsOverlay::PointsOverlay(void)
	: Overlay(0), mItemFirst(0), mItemLast(0)
{
}

// constructor
PointsOverlay::PointsOverlay(unsigned int aId)
	: Overlay(aId), mItemFirst(0), mItemLast(0)
{
	SetAction(Action(this, &PointsOverlay::Render));
}

// add an item
void PointsOverlay::AddItem(const Vector2 &aPosition, int aValue, int aCombo)
{
	// show if adding an item
	if (mItemFirst == mItemLast)
		Show();

	mItems[mItemLast].mPosition = aPosition;
	mItems[mItemLast].mValue = aValue;
	mItems[mItemLast].mCombo = aCombo;
	mItems[mItemLast].mTime = 2.0f;

	mItemLast = (mItemLast + 1) % SDL_arraysize(mItems);

	if (mItemFirst == mItemLast)
		mItemFirst = (mItemLast + 1) % SDL_arraysize(mItems);
}

// render
void PointsOverlay::Render(unsigned int aId, float aTime, const Transform2 &aTransform)
{
	// set projection
	glMatrixMode( GL_PROJECTION );
	glPushMatrix();
	glLoadIdentity();
	glFrustum( -0.5*VIEW_SIZE*SCREEN_WIDTH/SCREEN_HEIGHT, 0.5*VIEW_SIZE*SCREEN_WIDTH/SCREEN_HEIGHT, 0.5f*VIEW_SIZE, -0.5f*VIEW_SIZE, 256.0f*1.0f, 256.0f*5.0f );

	// set base modelview matrix
	glMatrixMode( GL_MODELVIEW );
	glPushMatrix();
	glLoadIdentity();
	glTranslatef( 0.0f, 0.0f, -256.0f );
	glScalef( -1.0f, -1.0f, -1.0f );

	// push camera transform
	glPushMatrix();

	// get interpolated track position
	Vector2 viewpos(Lerp(camerapos[0], camerapos[1], sim_fraction));

	// set view position
	glTranslatef( -viewpos.x, -viewpos.y, 0 );

	// start drawing
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, OGLCONSOLE_glFontHandle);
	glBegin(GL_QUADS);

	// for each points item
	for (int i = mItemFirst; i != mItemLast; i = (i + 1) % SDL_arraysize(mItems))
	{
		// get the item
		PointsItem &item = mItems[i];

		// get string
		char buf[16];
		if (item.mCombo > 1)
			sprintf(buf, "%dx%d", item.mValue, item.mCombo);
		else
			sprintf(buf, "%d", item.mValue);

		// draw point value
		glColor4f(1.0f, 1.0f, 1.0f, std::min(item.mTime, 1.0f));
		float w = 4 * VIEW_SIZE / 240;
		OGLCONSOLE_DrawString(buf, item.mPosition.x + w * 0.5f * strlen(buf), item.mPosition.y - w * 0.5f, -w, w, 0);

		// count down time
		item.mTime -= frame_time;

		// delete if expired
		if (item.mTime <= 0.0f)
			mItemFirst = (i + 1) % SDL_arraysize(mItems);
	}

	// finish drawing
	glEnd();
	glDisable(GL_TEXTURE_2D);

	// reset camera transform
	glPopMatrix();

	// reset camera transform
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

	// hide if empty...
	if (mItemFirst == mItemLast)
		Hide();
}

#include "StdAfx.h"
#include "PointsOverlay.h"
#include "Font.h"
#include "Render.h"
#include "MatrixStack.h"


extern float CAMERA_DISTANCE;


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
	// flush geometry
	FlushDynamic();

	// set projection
	ProjectionPush();
	ProjectionFrustum( -0.5f*VIEW_SIZE*SCREEN_WIDTH/SCREEN_HEIGHT, 0.5f*VIEW_SIZE*SCREEN_WIDTH/SCREEN_HEIGHT, 0.5f*VIEW_SIZE, -0.5f*VIEW_SIZE, 256.0f*1.0f, 256.0f*5.0f );

	// get interpolated track position
	Vector2 viewpos(Lerp(camerapos[0], camerapos[1], sim_fraction));

	// set view matrix
	StackPush();
	StackIdentity();
	StackScale(_mm_setr_ps(-1, -1, -1, 0));
	StackTranslate(_mm_setr_ps(-viewpos.x, -viewpos.y, CAMERA_DISTANCE, 0));
	ViewLoad(StackGet());

	// start drawing
	FontDrawBegin(sDefaultFontHandle);

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
		FontDrawColor(Color4(1.0f, 1.0f, 1.0f, std::min(item.mTime, 1.0f)));
		float w = 4 * VIEW_SIZE / 240;
		FontDrawString(buf, item.mPosition.x + w * 0.5f * strlen(buf), item.mPosition.y - w * 0.5f, -w, w, 0);

		// count down time
		item.mTime -= frame_time;

		// delete if expired
		if (item.mTime <= 0.0f)
			mItemFirst = (i + 1) % SDL_arraysize(mItems);
	}

	// finish drawing
	FontDrawEnd();

	// reset camera transform
	StackPop();
	ViewLoad(StackGet());
	ProjectionPop();

	// hide if empty...
	if (mItemFirst == mItemLast)
		Hide();
}

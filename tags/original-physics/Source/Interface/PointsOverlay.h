#pragma once

#include "Overlay.h"

// points overlay
class PointsOverlay : public Overlay
{
	struct PointsItem
	{
		Vector2 mPosition;
		int mValue : 24;
		int mCombo : 8;
		float mTime;
	};
	PointsItem mItems[256];
	int mItemFirst;
	int mItemLast;

public:
	PointsOverlay(void);
	PointsOverlay(unsigned int aId);

	void AddItem(const Vector2 &aPosition, int aValue, int aCombo);

	void Render(unsigned int aId, float aTime, const Transform2 &aTransform);
};

namespace Database
{
	extern Typed<PointsOverlay *> pointsoverlay;
}

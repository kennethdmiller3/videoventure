#pragma once

class PlayerResetTemplate
{
public:
	float mOffset;
};

class PlayerReset : public Updatable
{
public:
	PlayerReset(void);
	PlayerReset(const PlayerResetTemplate &aTemplate, unsigned int aId);
	void Update(float aStep);
	void OnDeath(unsigned int aId, unsigned int aSourceId);

protected:
	void Reset(Player *player);
};


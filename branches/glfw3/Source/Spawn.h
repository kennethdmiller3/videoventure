#pragma once

class SpawnTemplate
{
public:
	Transform2 mOffset;
	Transform2 mScatter;
	Transform2 mInherit;
	Transform2 mVelocity;
	Transform2 mVariance;
	unsigned int mSpawn;

public:
	SpawnTemplate(void);

	bool Configure(const tinyxml2::XMLElement *element, unsigned int aId);

	unsigned int Instantiate(unsigned int aId, const Transform2 &aPosition, const Transform2 &aVelocity) const;
};

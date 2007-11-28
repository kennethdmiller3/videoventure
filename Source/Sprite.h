#pragma once

class Sprite
{
protected:
	// texture handle
	GLuint texture;

	// texture coordinates
	GLfloat u0;
	GLfloat v0;
	GLfloat u1;
	GLfloat v1;

	// pixel coordinates
	GLfloat x0;
	GLfloat y0;
	GLfloat x1;
	GLfloat y1;

public:
	Sprite(void);
	~Sprite(void);

	// set texture
	void SetTexture(const char *aName);

	// set coords
	void SetCoords(float aU0, float aV0, float aU1, float aV1)
	{
		u0 = aU0;
		v0 = aV0;
		u1 = aU1;
		v1 = aV1;
	}

	// set rectancle
	void SetRect(float aX0, float aY0, float aX1, float aY1)
	{
		x0 = aX0;
		y0 = aY0;
		x1 = aX1;
		y1 = aY1;
	}

	// render
	void Render(void);
};

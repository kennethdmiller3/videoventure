#pragma once

// texture color modulate fog shader
// (multiply vertex color and texture color, apply fog based on depth)

namespace ShaderModulateFog
{
	extern void Init();
	extern void PreReset();
	extern void PostReset();
	extern void Cleanup();

	// shader program handle
	extern GLuint gProgramId;

	// uniform locations
	extern GLint gUniformModelViewProj;
	extern GLint gUniformModelView;
	extern GLint gUniformFogColor;
	extern GLint gUniformFogParams;

	// attribute locations
	extern GLint gAttribPosition;
	extern GLint gAttribColor;
	extern GLint gAttribTexCoord;
}

#pragma once

// texture color modulate shader
// (multiply vertex color and texture color)

namespace ShaderModulate
{
	extern void Init();
	extern void PreReset();
	extern void PostReset();
	extern void Cleanup();

	// shader program handle
	extern GLuint gProgramId;

	// uniform locations
	extern GLint gUniformModelViewProj;
}

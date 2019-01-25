#pragma once

// color fog shader
// (use vertex color, apply fog based on depth)

namespace ShaderColorFog
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
}

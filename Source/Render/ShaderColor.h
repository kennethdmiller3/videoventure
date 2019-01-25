#pragma once

// color shader
// (simple vertex color pass-through)

namespace ShaderColor
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

#pragma once

// get identity matrix
extern const float *IdentityGet(void);

// projection matrix operations
extern void ProjectionLoad(const float *values);
extern const float *ProjectionGet(void);

// matrix stack operations
extern void StackInit(void);
extern const float *StackGet(void);
extern void StackPush(void);
extern void StackPop(void);
extern void StackIdentity(void);
extern void StackLoad(const float *values);
extern void StackMult(const float *values);
extern void StackRotate(const float value);
extern void StackScale(const __m128 value);
extern void StackTranslate(const __m128 value);
extern __m128 StackTransformNormal(const __m128 value);
extern __m128 StackTransformPosition(const __m128 value);

#pragma once

// get identity matrix
extern const float *IdentityGet(void);

// projection matrix operations
extern void ProjectionPush(void);
extern void ProjectionPop(void);
extern void ProjectionIdentity(void);
extern void ProjectionLoad(const float *aValues);
extern void ProjectionMult(const float *aValues);
extern void ProjectionOrtho(float aLeft, float aRight, float aBottom, float aTop, float aNear, float aFar);
extern void ProjectionFrustum(float aLeft, float aRight, float aBottom, float aTop, float aNear, float aFar);
extern const float *ProjectionGet(void);

// view matrix operations
extern void ViewLoad(const float *aValues);
extern const float *ViewGet(void);

// matrix stack operations
extern void StackInit(void);
extern const float *StackGet(void);
extern void StackPush(void);
extern void StackPop(void);
extern void StackIdentity(void);
extern void StackLoad(const float *aValues);
extern void StackMult(const float *aValues);
extern void StackRotate(const float aValue);
extern void StackScale(const __m128 aValue);
extern void StackTranslate(const __m128 aValue);
extern __m128 StackTransformNormal(const __m128 aValue);
extern __m128 StackTransformPosition(const __m128 aValue);

// view-projection operations
extern bool ViewProjChanged(void);
extern const float *ViewProjGet(void);

// model-view-projection operations
// (sequence increments every time the matrix is invalidated)
extern bool ModelViewProjChanged(void);
extern const float *ModelViewProjGet(void);

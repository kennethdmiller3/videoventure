#pragma once

class Entity;

extern Vector2 Intercept(float aLeading, const Vector2 &aPosition, const Vector2 &aVelocity);
extern Vector2 TargetDir(float aLeading, const Entity *aEntity, const Entity *aTargetEntity, const Vector2 &aOffset);

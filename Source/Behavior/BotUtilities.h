#pragma once

class Entity;

extern GAME_API Vector2 Intercept(float aLeading, const Vector2 &aPosition, const Vector2 &aVelocity);
extern GAME_API Vector2 TargetDir(float aLeading, const Entity *aEntity, const Entity *aTargetEntity, const Vector2 &aOffset);
extern GAME_API float SteerTowards(const Entity *aEntity, const Vector2 &aLocalDir);

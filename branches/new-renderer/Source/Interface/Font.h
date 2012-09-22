#pragma once

extern GLuint sDefaultFontHandle;

extern void InitFonts();
extern void PreResetFonts(void);
extern void PostResetFonts(void);
extern void CleanupFonts();

extern int FontGetWidth(GLuint handle, int c);
extern int FontGetHeight(GLuint handle);
extern void FontDrawBegin(GLuint handle);
extern void FontDrawEnd();
extern void FontDrawColor(const Color4 &color);
extern void FontDrawCharacter(int c, float x, float y, float w, float h, float z);
extern void FontDrawString(const char *s, float x, float y, float w, float h, float z, float wrap = FLT_MAX);

#pragma once

//
// TYPE SCHEMA
//

namespace Expression
{
	template <typename T> struct Schema { static const char * const NAME; };
	template <> struct GAME_API Schema<bool> { enum { COUNT = 1 }; static const char * const NAME; };
	template <> struct GAME_API Schema<float> { enum { COUNT = 1 }; static const char * const NAME; };
	template <> struct GAME_API Schema<float const> { enum { COUNT = 1 }; static const char * const NAME; };
	template <> struct GAME_API Schema<Vector2> { enum { COUNT = 2 }; static const char *const NAME; };
	template <> struct GAME_API Schema<Vector2 const> { enum { COUNT = 2 }; static const char *const NAME; };
	template <> struct GAME_API Schema<Vector3> { enum { COUNT = 3 }; static const char *const NAME; };
	template <> struct GAME_API Schema<Vector3 const> { enum { COUNT = 3 }; static const char *const NAME; };
	template <> struct GAME_API Schema<Vector4> { enum { COUNT = 4 }; static const char *const NAME; };
	template <> struct GAME_API Schema<Vector4 const> { enum { COUNT = 4 }; static const char *const NAME; };
	template <> struct GAME_API Schema<Color4> { enum { COUNT = 4 }; static const char *const NAME; };
	template <> struct GAME_API Schema<Color4 const> { enum { COUNT = 4 }; static const char *const NAME; };
	template <> struct GAME_API Schema<__m128> { enum { COUNT = 4 }; static const char *const NAME; };
	template <> struct GAME_API Schema<__m128 const> { enum { COUNT = 4 }; static const char *const NAME; };
}
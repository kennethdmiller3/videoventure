#pragma once

//
// TYPE SCHEMA
//

namespace Expression
{
	template <typename T> struct Schema { static const char * const NAME; };
	template <> struct Schema<bool> { enum { COUNT = 1 }; static const char * const NAME; };
	template <> struct Schema<float> { enum { COUNT = 1 }; static const char * const NAME; };
	template <> struct Schema<float const> { enum { COUNT = 1 }; static const char * const NAME; };
	template <> struct Schema<Vector2> { enum { COUNT = 2 }; static const char *const NAME; };
	template <> struct Schema<Vector2 const> { enum { COUNT = 2 }; static const char *const NAME; };
	template <> struct Schema<Vector3> { enum { COUNT = 3 }; static const char *const NAME; };
	template <> struct Schema<Vector3 const> { enum { COUNT = 3 }; static const char *const NAME; };
	template <> struct Schema<Vector4> { enum { COUNT = 4 }; static const char *const NAME; };
	template <> struct Schema<Vector4 const> { enum { COUNT = 4 }; static const char *const NAME; };
	template <> struct Schema<Color4> { enum { COUNT = 4 }; static const char *const NAME; };
	template <> struct Schema<Color4 const> { enum { COUNT = 4 }; static const char *const NAME; };
	template <> struct Schema<__m128> { enum { COUNT = 4 }; static const char *const NAME; };
	template <> struct Schema<__m128 const> { enum { COUNT = 4 }; static const char *const NAME; };
}
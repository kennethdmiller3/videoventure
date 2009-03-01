#include "StdAfx.h"
#include "Drawlist.h"
#include "Texture.h"
#include "Interpolator.h"
#include "Noise.h"

#include "Expression.h"

#define DRAWLIST_LOOP

#include "xmmintrin.h"
//#include "fvec.h"


// draw item context
// (extends expression context)
struct DrawItemContext : public Expression::Context
{
	const unsigned int *mEnd;
	float mParam;
	unsigned int mId;
	Database::Typed<float> *mVars;
};

// execute a dynamic draw list
void ExecuteDrawItems(DrawItemContext &aContext);


//
// DATA TYPES
//

typedef float DLScalar;
static const char * const sScalarNames[] = { "value" };
static const float sScalarDefault[] = { 0.0f };
static const int sScalarWidth = 1;

//typedef Vector3 DLPosition;
typedef __m128 DLPosition;
static const char * const sPositionNames[] = { "x", "y", "z", "w" };
static const float sPositionDefault[] = { 0.0f, 0.0f, 0.0f, 1.0f };
static const int sPositionWidth = 3;

//typedef Vector3 DLNormal;
typedef __m128 DLNormal;
static const char * const sNormalNames[] = { "x", "y", "z", "w" };
static const float sNormalDefault[] = { 0.0f, 0.0f, 0.0f, 0.0f };
static const int sNormalWidth = 3;

//typedef Vector3 DLTranslation;
typedef __m128 DLTranslation;
static const char * const sTranslationNames[] = { "x", "y", "z", "w" };
static const float sTranslationDefault[] = { 0.0f, 0.0f, 0.0f, 1.0f };
static const int sTranslationWidth = 3;

typedef float DLRotation;
static const char * const sRotationNames[] = { "angle" };
static const float sRotationDefault[] = { 0.0f };
static const int sRotationWidth = 1;

//typedef Vector3 DLScale;
typedef __m128 DLScale;
static const char * const sScaleNames[] = { "x", "y", "z", "w" };
static const float sScaleDefault[] = { 1.0f, 1.0f, 1.0f, 1.0f };
static const int sScaleWidth = 3;

//typedef Color4 DLColor;
typedef __m128 DLColor;
static const char * const sColorNames[] = { "r", "g", "b", "a" };
static const float sColorDefault[] = { 0.0f, 0.0f, 0.0f, 1.0f };
static const int sColorWidth = 4;

//typedef Vector2 DLTexCoord;
typedef __m128 DLTexCoord;
static const char * const sTexCoordNames[] = { "s", "t", "r", "q" };
static const float sTexCoordDefault[] = { 0.0f, 0.0f, 0.0f, 1.0f };
static const int sTexCoordWidth = 2;

typedef float DLIndex;
static const char * const sIndexNames[] = { "c" };
static const float sIndexDefault[] = { 0.0f };
static const int sIndexWidth = 1;

static const char * const sMatrixNames[] = { "m0", "m1", "m2", "m3", "m4", "m5", "m6", "m7", "m8", "m9", "m10", "m11", "m12", "m13", "m14", "m15" };
static const float sMatrixDefault[] = { 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f };
static const int sMatrixWidth = 16;

void GetTypeData(unsigned int type, int &width, const char * const *&names, const float *&data)
{
	switch (type)
	{
	default:
	case 0x934f4e0a /* "position" */:	names = sPositionNames; data = sPositionDefault; width = sPositionWidth; break;
	case 0xe68b9c52 /* "normal" */:		names = sNormalNames; data = sNormalDefault; width = sNormalWidth; break;
	case 0xad0ecfd5 /* "translate" */:	names = sTranslationNames, data = sTranslationDefault; width = sTranslationWidth; break;
	case 0x21ac415f /* "rotation" */:	names = sRotationNames; data = sRotationDefault; width = sRotationWidth; break;
	case 0x82971c71 /* "scale" */:		names = sScaleNames; data = sScaleDefault; width = sScaleWidth; break;
	case 0x3d7e6258 /* "color" */:		names = sColorNames; data = sColorDefault; width = sColorWidth; break;
	case 0xdd612dd3 /* "texcoord" */:	names = sTexCoordNames; data = sTexCoordDefault; width = sTexCoordWidth; break;
	case 0x090aa9ab /* "index" */:		names = sIndexNames; data = sIndexDefault; width = sIndexWidth; break;
	case 0x15c2f8ec /* "matrix" */:		names = sMatrixNames; data = sMatrixDefault; width = sMatrixWidth; break;
	}
}

namespace Database
{
	Typed<std::vector<unsigned int> > dynamicdrawlist(0xdf3cf9c0 /* "dynamicdrawlist" */);
	Typed<GLuint> drawlist(0xc98b019b /* "drawlist" */);
	Typed<Typed<float> > variable(0x19385305 /* "variable" */);

	namespace Loader
	{
		class DynamicDrawlistLoader
		{
		public:
			DynamicDrawlistLoader()
			{
				AddConfigure(0xdf3cf9c0 /* "dynamicdrawlist" */, Entry(this, &DynamicDrawlistLoader::Configure));
			}

			void Configure(unsigned int aId, const TiXmlElement *element)
			{
				std::vector<unsigned int> &buffer = Database::dynamicdrawlist.Open(aId);
				ConfigureDrawItems(element, buffer);
				Database::dynamicdrawlist.Close(aId);
			}
		}
		dynamicdrawlistloader;

		class DrawlistLoader
		{
		public:
			DrawlistLoader()
			{
				AddConfigure(0xc98b019b /* "drawlist" */, Entry(this, &DrawlistLoader::Configure));
			}

			void Configure(unsigned int aId, const TiXmlElement *element)
			{
				// create a new draw list
				GLuint handle = glGenLists(1);
				glNewList(handle, GL_COMPILE);

				// register the draw list
				Database::drawlist.Put(aId, handle);

				// get (optional) parameter value
				float param = 0.0f;
				element->QueryFloatAttribute("param", &param);

				// configure the dynamic draw list
				std::vector<unsigned int> &drawlist = Database::dynamicdrawlist.Open(handle);
				ConfigureDrawItems(element, drawlist);

				// execute the dynamic draw list
				DrawItemContext context;
				context.mStream = &drawlist[0];
				context.mEnd = context.mStream + drawlist.size();
				context.mParam = param;
				context.mId = aId;
				context.mVars = &Database::variable.Open(aId);
				ExecuteDrawItems(context);
				Database::variable.Close(aId);

				// close the dynamic draw list
				Database::dynamicdrawlist.Close(handle);

				// finish the draw list
				glEndList();
			}
		}
		drawlistloader;

		class VariableLoader
		{
		public:
			VariableLoader()
			{
				AddConfigure(0x19385305 /* "variable" */, Entry(this, &VariableLoader::Configure));
			}

			void Configure(unsigned int aId, const TiXmlElement *element)
			{
				Typed<float> &variables = Database::variable.Open(aId);

				unsigned int name = Hash(element->Attribute("name"));
				unsigned int type = Hash(element->Attribute("type"));
				int width;
				const char * const *names;
				const float *data;
				GetTypeData(type, width, names, data);
				for (int i = 0; i < width; i++)
				{
					float value = data[i];
					element->QueryFloatAttribute(names[i], &value);
					variables.Put(name+i, value);
				}

				Database::variable.Close(aId);
			}
		}
		variableloader;
	}

}

#if 0
static const unsigned int sHashToAttribMask[][2] =
{
	{ 0xd965bbda /* "current" */,			GL_CURRENT_BIT },
	{ 0x18ae6c91 /* "point" */,				GL_POINT_BIT },
	{ 0x17db1627 /* "line" */,				GL_LINE_BIT },
	{ 0x051cb889 /* "polygon" */,			GL_POLYGON_BIT },
	{ 0x67b14997 /* "polygon_stipple" */,	GL_POLYGON_STIPPLE_BIT },
	{ 0xccde91eb /* "pixel_mode" */,		GL_LIGHTING_BIT },
	{ 0x827eb1c9 /* "lighting" */,			GL_POINT_BIT },
	{ 0xa1f3723f /* "fog" */,				GL_FOG_BIT },
	{ 0x65e5b825 /* "depth_buffer" */,		GL_DEPTH_BUFFER_BIT },
	{ 0x907f6213 /* "accum_buffer" */,		GL_ACCUM_BUFFER_BIT },
	{ 0x632020be /* "stencil_buffer" */,	GL_STENCIL_BUFFER_BIT },
	{ 0xe4abbac3 /* "viewport" */,			GL_VIEWPORT_BIT },
	{ 0xe1ad931b /* "transform" */,			GL_TRANSFORM_BIT },
	{ 0xaf8bb8ce /* "enable" */,			GL_ENABLE_BIT },
	{ 0x0d759bbb /* "color_buffer" */,		GL_COLOR_BUFFER_BIT },
	{ 0x4bc809b8 /* "hint" */,				GL_HINT_BIT },
	{ 0x08d22e0f /* "eval" */,				GL_EVAL_BIT },
	{ 0x0cfb5881 /* "list" */,				GL_LIST_BIT },
	{ 0x3c6468f4 /* "texture" */,			GL_TEXTURE_BIT },
	{ 0x0adbc081 /* "scissor" */,			GL_SCISSOR_BIT },
};
#endif

template <typename T> void ConfigureExpression(const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float data[]);

namespace Expression
{
	// read a value from an expression stream
	template <> inline const __m128 Read<__m128>(Context &aContext)
	{
		__m128 value(_mm_loadu_ps(reinterpret_cast<const float *>(aContext.mStream)));
		aContext.mStream += (sizeof(__m128) + sizeof(unsigned int) - 1) / sizeof(unsigned int);
		return value;
	}
	template <> __m128 *New<__m128, __m128>(std::vector<unsigned int> &aBuffer, __m128 aArg1)
	{
		void *ptr = Alloc(aBuffer, sizeof(__m128));
		_mm_storeu_ps(reinterpret_cast<float *>(ptr), aArg1);
		return reinterpret_cast<__m128 *>(ptr);
	}
	template <> struct ComponentNullary<__m128, 4>
	{
		// requres that T support operator[]
		template <typename OR, OR Op()> static const __m128 Evaluate(Context &aContext)
		{
			return _mm_set_ps(Op(), Op(), Op(), Op());
		}
	};
	template <> struct ComponentUnary<__m128, 4>
	{
		// requres that T support operator[]
		template <typename OR, typename O1, OR Op(O1)> static const __m128 Evaluate(Context &aContext)
		{
			__m128 arg1(Expression::Evaluate<__m128>(aContext));
			return _mm_setr_ps(
				Op(reinterpret_cast<float *>(&arg1)[0]),
				Op(reinterpret_cast<float *>(&arg1)[1]),
				Op(reinterpret_cast<float *>(&arg1)[2]),
				Op(reinterpret_cast<float *>(&arg1)[3])
				);
		}
	};
	template <> struct ComponentBinary<__m128, 4>
	{
		// requres that T support operator[]
		template <typename OR, typename O1, typename O2, OR Op(O1, O2)> static const __m128 Evaluate(Context &aContext)
		{
			__m128 arg1(Expression::Evaluate<__m128>(aContext));
			__m128 arg2(Expression::Evaluate<__m128>(aContext));
			return _mm_setr_ps(
				Op(reinterpret_cast<float *>(&arg1)[0], reinterpret_cast<float *>(&arg2)[0]),
				Op(reinterpret_cast<float *>(&arg1)[1], reinterpret_cast<float *>(&arg2)[1]),
				Op(reinterpret_cast<float *>(&arg1)[2], reinterpret_cast<float *>(&arg2)[2]),
				Op(reinterpret_cast<float *>(&arg1)[3], reinterpret_cast<float *>(&arg2)[3])
				);
		}
	};
	template <> struct ComponentTernary<__m128, 4>
	{
		// requres that T support operator[]
		template <typename OR, typename O1, typename O2, typename O3, OR Op(O1, O2, O3)> static const __m128 Evaluate(Context &aContext)
		{
			__m128 arg1(Expression::Evaluate<__m128>(aContext));
			__m128 arg2(Expression::Evaluate<__m128>(aContext));
			__m128 arg3(Expression::Evaluate<__m128>(aContext));
			return _mm_setr_ps(
				Op(reinterpret_cast<float *>(&arg1)[0], reinterpret_cast<float *>(&arg2)[0], reinterpret_cast<float *>(&arg3)[0]),
				Op(reinterpret_cast<float *>(&arg1)[1], reinterpret_cast<float *>(&arg2)[1], reinterpret_cast<float *>(&arg3)[1]),
				Op(reinterpret_cast<float *>(&arg1)[2], reinterpret_cast<float *>(&arg2)[2], reinterpret_cast<float *>(&arg3)[2]),
				Op(reinterpret_cast<float *>(&arg1)[3], reinterpret_cast<float *>(&arg2)[3], reinterpret_cast<float *>(&arg3)[3])
				);
		}
	};

	// schema
	template <typename T> struct Schema { static const char * const NAME; };
	template <> struct Schema<float> { enum { COUNT = 0 }; static const char * const NAME; };
	const char * const Schema<float>::NAME = "float";
	template <> struct Schema<float const> { enum { COUNT = 0 }; static const char * const NAME; };
	const char * const Schema<float const>::NAME = "float const";
	template <> struct Schema<Vector2> { enum { COUNT = 2 }; static const char *const NAME; };
	const char * const Schema<Vector2>::NAME = "Vector2";
	template <> struct Schema<Vector3> { enum { COUNT = 3 }; static const char *const NAME; };
	const char * const Schema<Vector3>::NAME = "Vector3";
	template <> struct Schema<Vector4> { enum { COUNT = 4 }; static const char *const NAME; };
	const char * const Schema<Vector4>::NAME = "Vector4";
	template <> struct Schema<Color4> { enum { COUNT = 4 }; static const char *const NAME; };
	const char * const Schema<Color4>::NAME = "Color4";
	template <> struct Schema<__m128> { enum { COUNT = 4 }; static const char *const NAME; };
	const char * const Schema<__m128>::NAME = "__m128";
	template <> struct Schema<__m128 const> { enum { COUNT = 4 }; static const char *const NAME; };
	const char * const Schema<__m128 const>::NAME = "__m128 const";

	// various constructors
	template <typename T> const T Construct(Context &aContext);
	template <> const float Construct<float>(Context &aContext)
	{
		return Evaluate<float>(aContext);
	}
	template <> const Vector2 Construct<Vector2>(Context &aContext)
	{
		float arg1(Evaluate<float>(aContext));
		float arg2(Evaluate<float>(aContext));
		return Vector2(arg1, arg2);
	}
	template <> const Vector3 Construct<Vector3>(Context &aContext)
	{
		float arg1(Evaluate<float>(aContext));
		float arg2(Evaluate<float>(aContext));
		float arg3(Evaluate<float>(aContext));
		return Vector3(arg1, arg2, arg3);
	}
	template <> const Vector4 Construct<Vector4>(Context &aContext)
	{
		float arg1(Evaluate<float>(aContext));
		float arg2(Evaluate<float>(aContext));
		float arg3(Evaluate<float>(aContext));
		float arg4(Evaluate<float>(aContext));
		return Vector4(arg1, arg2, arg3, arg4);
	}
	template <> const Color4 Construct<Color4>(Context &aContext)
	{
		float arg1(Evaluate<float>(aContext));
		float arg2(Evaluate<float>(aContext));
		float arg3(Evaluate<float>(aContext));
		float arg4(Evaluate<float>(aContext));
		return Color4(arg1, arg2, arg3, arg4);
	}
	template <> const __m128 Construct<__m128>(Context &aContext)
	{
		float arg1(Evaluate<float>(aContext));
		float arg2(Evaluate<float>(aContext));
		float arg3(Evaluate<float>(aContext));
		float arg4(Evaluate<float>(aContext));
		return _mm_setr_ps(arg1, arg2, arg3, arg4);
	}

	// extend a scalar
	template <typename T, typename A> const T Extend(Context &aContext);
	template <> const float Extend<float, float>(Context &aContext)
	{
		return Evaluate<float>(aContext);
	}
	template <> const Vector2 Extend<Vector2, float>(Context &aContext)
	{
		float arg(Evaluate<float>(aContext));
		return Vector2(arg, arg);
	}
	template <> const Vector3 Extend<Vector3, float>(Context &aContext)
	{
		float arg(Evaluate<float>(aContext));
		return Vector3(arg, arg, arg);
	}
	template <> const Vector4 Extend<Vector4, float>(Context &aContext)
	{
		float arg(Evaluate<float>(aContext));
		return Vector4(arg, arg, arg, arg);
	}
	template <> const Color4 Extend<Color4, float>(Context &aContext)
	{
		float arg(Evaluate<float>(aContext));
		return Color4(arg, arg, arg, arg);
	}
	template <> const __m128 Extend<__m128, float>(Context &aContext)
	{
		float arg(Evaluate<float>(aContext));
		return _mm_set_ps1(arg);
	}

	// aritmetic operators
	template <typename T> T Add(Context &aContext)
	{
		T arg1(Evaluate<T>(aContext));
		T arg2(Evaluate<T>(aContext));
		return arg1 + arg2;
	}
	template <> __m128 Add(Context &aContext)
	{
		__m128 arg1(Evaluate<__m128>(aContext));
		__m128 arg2(Evaluate<__m128>(aContext));
		return _mm_add_ps(arg1, arg2);
	}
	template <typename T> T Sub(Context &aContext)
	{
		T arg1(Evaluate<T>(aContext));
		T arg2(Evaluate<T>(aContext));
		return arg1 - arg2;
	}
	template <> __m128 Sub(Context &aContext)
	{
		__m128 arg1(Evaluate<__m128>(aContext));
		__m128 arg2(Evaluate<__m128>(aContext));
		return _mm_sub_ps(arg1, arg2);
	}
	template <typename T> T Mul(Context &aContext)
	{
		T arg1(Evaluate<T>(aContext));
		T arg2(Evaluate<T>(aContext));
		return arg1 * arg2;
	}
	template <> __m128 Mul(Context &aContext)
	{
		__m128 arg1(Evaluate<__m128>(aContext));
		__m128 arg2(Evaluate<__m128>(aContext));
		return _mm_mul_ps(arg1, arg2);
	}
	template <typename T> T Div(Context &aContext)
	{
		T arg1(Evaluate<T>(aContext));
		T arg2(Evaluate<T>(aContext));
		return arg1 / arg2;
	}
	template <> __m128 Div(Context &aContext)
	{
		__m128 arg1(Evaluate<__m128>(aContext));
		__m128 arg2(Evaluate<__m128>(aContext));
		return _mm_div_ps(arg1, arg2);
	}
	template <typename T> T Neg(Context &aContext)
	{
		T arg1(Evaluate<T>(aContext));
		return -arg1;
	}
	template <> __m128 Neg(Context &aContext)
	{
		__m128 arg1(Evaluate<__m128>(aContext));
		return _mm_sub_ps(_mm_setzero_ps(), arg1);
	}
	float Rcp(float v) { return 1.0f / v; };
	template <typename T> T Rcp(Context &aContext)
	{
		return ComponentUnary<T, Schema<T>::COUNT>::Evaluate<float, float, Rcp>(aContext);
	}
	template <> __m128 Rcp(Context &aContext)
	{
		__m128 arg1(Evaluate<__m128>(aContext));
		return _mm_rcp_ps(arg1);
	}
	float Inc(float v) { return v + 1.0f; };
	template <typename T> T Inc(Context &aContext)
	{
		return ComponentUnary<T, Schema<T>::COUNT>::Evaluate<float, float, Inc>(aContext);
	}
	template <> __m128 Inc(Context &aContext)
	{
		__m128 arg1(Evaluate<__m128>(aContext));
		return _mm_add_ps(arg1, _mm_set_ps1(1));
	}
	float Dec(float v) { return v - 1.0f; };
	template <typename T> T Dec(Context &aContext)
	{
		return ComponentUnary<T, Schema<T>::COUNT>::Evaluate<float, float, Dec>(aContext);
	}
	template <> __m128 Dec(Context &aContext)
	{
		__m128 arg1(Evaluate<__m128>(aContext));
		return _mm_sub_ps(arg1, _mm_set_ps1(1));
	}
	
	// relational operators
	template <typename T> bool Greater(Context &aContext)
	{
		T arg1(Evaluate<T>)(aContext);
		T arg2(Evaluate<T>)(aContext);
		return arg1 > arg2;
	}
	template <typename T> bool GreaterEqual(Context &aContext)
	{
		T arg1(Evaluate<T>)(aContext);
		T arg2(Evaluate<T>)(aContext);
		return arg1 >= arg2;
	}
	template <typename T> bool Less(Context &aContext)
	{
		T arg1(Evaluate<T>)(aContext);
		T arg2(Evaluate<T>)(aContext);
		return arg1 < arg2;
	}
	template <typename T> bool LessEqual(Context &aContext)
	{
		T arg1(Evaluate<T>)(aContext);
		T arg2(Evaluate<T>)(aContext);
		return arg1 <= arg2;
	}
	template <typename T> bool Equal(Context &aContext)
	{
		T arg1(Evaluate<T>)(aContext);
		T arg2(Evaluate<T>)(aContext);
		return arg1 == arg2;
	}
	template <typename T> bool NotEqual(Context &aContext)
	{
		T arg1(Evaluate<T>)(aContext);
		T arg2(Evaluate<T>)(aContext);
		return arg1 != arg2;
	}

	// trigonometric functions
	template <typename T> T Sin(Context &aContext)
	{
		return ComponentUnary<T, Schema<T>::COUNT>::Evaluate<float, float, sinf>(aContext);
	}
	template <typename T> T Cos(Context &aContext)
	{
		return ComponentUnary<T, Schema<T>::COUNT>::Evaluate<float, float, cosf>(aContext);
	}
	template <typename T> T Tan(Context &aContext)
	{
		return ComponentUnary<T, Schema<T>::COUNT>::Evaluate<float, float, tanf>(aContext);
	}
	template <typename T> T Asin(Context &aContext)
	{
		return ComponentUnary<T, Schema<T>::COUNT>::Evaluate<float, float, asinf>(aContext);
	}
	template <typename T> T Acos(Context &aContext)
	{
		return ComponentUnary<T, Schema<T>::COUNT>::Evaluate<float, float, acosf>(aContext);
	}
	template <typename T> T Atan(Context &aContext)
	{
		return ComponentUnary<T, Schema<T>::COUNT>::Evaluate<float, float, atanf>(aContext);
	}
	template <typename T> T Atan2(Context &aContext)
	{
		return ComponentBinary<T, Schema<T>::COUNT>::Evaluate<float, float, float, atan2f>(aContext);
	}

	// hyperbolic functions
	template <typename T> T Sinh(Context &aContext)
	{
		return ComponentUnary<T, Schema<T>::COUNT>::Evaluate<float, float, sinhf>(aContext);
	}
	template <typename T> T Cosh(Context &aContext)
	{
		return ComponentUnary<T, Schema<T>::COUNT>::Evaluate<float, float, coshf>(aContext);
	}
	template <typename T> T Tanh(Context &aContext)
	{
		return ComponentUnary<T, Schema<T>::COUNT>::Evaluate<float, float, tanhf>(aContext);
	}

	// exponential functions
	template <typename T> T Pow(Context &aContext)
	{
		return ComponentBinary<T, Schema<T>::COUNT>::Evaluate<float, float, float, powf>(aContext);
	}
	template <typename T> T Exp(Context &aContext)
	{
		return ComponentUnary<T, Schema<T>::COUNT>::Evaluate<float, float, expf>(aContext);
	}
	template <typename T> T Log(Context &aContext)
	{
		return ComponentUnary<T, Schema<T>::COUNT>::Evaluate<float, float, logf>(aContext);
	}
	template <typename T> T Sqrt(Context &aContext)
	{
		return ComponentUnary<T, Schema<T>::COUNT>::Evaluate<float, float, sqrtf>(aContext);
	}
	template <> __m128 Sqrt(Context &aContext)
	{
		__m128 arg1(Evaluate<__m128>(aContext));
		return _mm_sqrt_ps(arg1);
	}
	template <typename T> T InvSqrt(Context &aContext)
	{
		return ComponentUnary<T, Schema<T>::COUNT>::Evaluate<float, float, ::InvSqrt>(aContext);
	}
	template <> __m128 InvSqrt(Context &aContext)
	{
		__m128 arg1(Evaluate<__m128>(aContext));
		return _mm_rsqrt_ps(arg1);
	}

	// common functions
	template <typename T> T Abs(Context &aContext)
	{
		return ComponentUnary<T, Schema<T>::COUNT>::Evaluate<float, float, fabsf>(aContext);
	}
	template <> __m128 Abs<__m128>(Context &aContext)
	{
		__m128 arg1(Evaluate<__m128>(aContext));
		return _mm_max_ps(arg1, _mm_sub_ps(_mm_setzero_ps(), arg1));
	}
	float Sign(float v) { return (v == 0) ? (0.0f) : ((v > 0) ? (1.0f) : (-1.0f)); }
	template <typename T> T Sign(Context &aContext)
	{
		return ComponentUnary<T, Schema<T>::COUNT>::Evaluate<float, float, Sign>(aContext);
	}
	template <typename T> T Floor(Context &aContext)
	{
		return ComponentUnary<T, Schema<T>::COUNT>::Evaluate<float, float, floorf>(aContext);
	}
	template <typename T> T Ceil(Context &aContext)
	{
		return ComponentUnary<T, Schema<T>::COUNT>::Evaluate<float, float, ceilf>(aContext);
	}
	float Frac(float v) { return v - xs_FloorToInt(v); }
	template <typename T> T Frac(Context &aContext)
	{
		return ComponentUnary<T, Schema<T>::COUNT>::Evaluate<float, float, Frac>(aContext);
	}
	template <typename T> T Mod(Context &aContext)
	{
		return ComponentBinary<T, Schema<T>::COUNT>::Evaluate<float, float, float, fmodf>(aContext);
	}
	template <typename T> T Min(Context &aContext)
	{
		return ComponentBinary<T, Schema<T>::COUNT>::Evaluate<const float &, const float &, const float &, std::min<float> >(aContext);
	}
	template <> __m128 Min(Context &aContext)
	{
		__m128 arg1(Evaluate<__m128>(aContext));
		__m128 arg2(Evaluate<__m128>(aContext));
		return _mm_min_ps(arg1, arg2);
	}
	template <typename T> T Max(Context &aContext)
	{
		return ComponentBinary<T, Schema<T>::COUNT>::Evaluate<const float &, const float &, const float &, std::max<float> >(aContext);
	}
	template <> __m128 Max(Context &aContext)
	{
		__m128 arg1(Evaluate<__m128>(aContext));
		__m128 arg2(Evaluate<__m128>(aContext));
		return _mm_max_ps(arg1, arg2);
	}
	template <typename T> T Clamp(Context &aContext)
	{
		return ComponentTernary<T, Schema<T>::COUNT>::Evaluate<const float, const float, const float, const float, ::Clamp<float> >(aContext);
	}
	template <> __m128 Clamp(Context &aContext)
	{
		__m128 val(Evaluate<__m128>(aContext));
		__m128 min(Evaluate<__m128>(aContext));
		__m128 max(Evaluate<__m128>(aContext));
		return _mm_min_ps(_mm_max_ps(val, min), max);
	}
	template <typename T> T Lerp(Context &aContext)
	{
		return Ternary<T, T, T, float>::Evaluate<const T, const T, const T, float, ::Lerp<T> >(aContext);
	}
	template <> __m128 Lerp(Context &aContext)
	{
		__m128 v0(Evaluate<__m128>(aContext));
		__m128 v1(Evaluate<__m128>(aContext));
		float s(Evaluate<float>(aContext));
		return _mm_add_ps(v0, _mm_mul_ps(_mm_sub_ps(v1, v0), _mm_set_ps1(s)));
	}
	float Step(float e, float v) { return v < e ? 0.0f : 1.0f; }
	template <typename T> T Step(Context &aContext)
	{
		return ComponentBinary<T, Schema<T>::COUNT>::Evaluate<float, float, float, Step >(aContext);
	}
	float SmoothStep(float e0, float e1, float v)
	{
		if (v <= e0) return 0.0f;
		if (v >= e1) return 1.0f;
		float t = (v - e0) / (e1 - e0);
		return t * t * (3 - 2 * t);
	}
	template <typename T> T SmoothStep(Context &aContext)
	{
		return ComponentTernary<T, Schema<T>::COUNT>::Evaluate<float, float, float, float, SmoothStep >(aContext);
	}
}


//
// VARIABLE OPERATOR
//

// component-level operators
typedef void (* VariableOperator)(float &, float);
void VariableOperatorSet(float &v, float data) { v = data; }
void VariableOperatorAdd(float &v, float data) { v += data; }
void VariableOperatorSub(float &v, float data) { v -= data; }
void VariableOperatorMul(float &v, float data) { v *= data; }
void VariableOperatorDiv(float &v, float data) { v /= data; }
void VariableOperatorMin(float &v, float data) { v = std::min(v, data); }
void VariableOperatorMax(float &v, float data) { v = std::max(v, data); }

// evaluate variable operator
bool EvaluateVariableOperator(DrawItemContext &aContext, VariableOperator op)
{
	unsigned int name = *aContext.mStream++;
	int width = *aContext.mStream++;
	assert(width <= 4);
	__m128 value;
	switch(width)
	{
	case 1: value = _mm_set_ps1(Expression::Evaluate<float>(aContext)); break;
	case 2: //value = Expression::Evaluate<Vector2>(aContext); break;
	case 3: //value = Expression::Evaluate<Vector3>(aContext); break;
	case 4: value = Expression::Evaluate<__m128>(aContext); break;
	}
//	Database::Typed<float> &variables = Database::variable.Open(aContext.mId);
	for (int i = 0; i < width; i++)
	{
		float &v = aContext.mVars->Open(name+i);
		op(v, reinterpret_cast<float *>(&value)[i]);
		aContext.mVars->Close(name+i);
	}
//	Database::variable.Close(aContext.mId);
	return true;
}



//
// DRAWLIST OPERATIONS
//

void DO_glArrayElement(DrawItemContext &aContext)
{
	glArrayElement(Expression::Read<GLint>(aContext));
}

void DO_glBegin(DrawItemContext &aContext)
{
	glBegin(Expression::Read<GLenum>(aContext));
}

void DO_glBindTexture(DrawItemContext &aContext)
{
	GLenum target(Expression::Read<GLenum>(aContext));
	GLuint texture(Expression::Read<GLuint>(aContext));
	glBindTexture(target, texture);
}

void DO_glBlendFunc(DrawItemContext &aContext)
{
	GLenum sfactor(Expression::Read<GLenum>(aContext));
	GLenum dfactor(Expression::Read<GLenum>(aContext));
	glBlendFunc(sfactor, dfactor);
}

void DO_glCallList(DrawItemContext &aContext)
{
	glCallList(Expression::Read<GLuint>(aContext));
}

void DO_glColor4fv(DrawItemContext &aContext)
{
	DLColor value(Expression::Evaluate<DLColor>(aContext));
	glColor4fv(reinterpret_cast<float *>(&value));
}

void DO_glColorPointer(DrawItemContext &aContext)
{
	GLint size(Expression::Read<GLint>(aContext));
	GLsizei stride(Expression::Read<GLsizei>(aContext));
	size_t count(Expression::Read<size_t>(aContext));
	glColorPointer(size, GL_FLOAT, stride, aContext.mStream);
	aContext.mStream += (count*sizeof(GLfloat)+sizeof(unsigned int)-1)/sizeof(unsigned int);
}

void DO_glDisable(DrawItemContext &aContext)
{
	glDisable(Expression::Read<GLenum>(aContext));
}

void DO_glDisableClientState(DrawItemContext &aContext)
{
	glDisableClientState(Expression::Read<GLenum>(aContext));
}

void DO_glDrawArrays(DrawItemContext &aContext)
{
	GLenum mode(Expression::Read<GLenum>(aContext));
	GLint first(Expression::Read<GLint>(aContext));
	size_t count(Expression::Read<size_t>(aContext));
	glDrawArrays(mode, first, count);
}

void DO_glDrawElements(DrawItemContext &aContext)
{
	GLenum mode(Expression::Read<GLenum>(aContext));
	size_t count(Expression::Read<size_t>(aContext));
	glDrawElements(mode, count, GL_UNSIGNED_SHORT, aContext.mStream);
	aContext.mStream += (count*sizeof(unsigned short)+sizeof(unsigned int)-1)/sizeof(unsigned int);
}

void DO_glEdgeFlag(DrawItemContext &aContext)
{
	glEdgeFlag(Expression::Read<GLboolean>(aContext));
}

void DO_glEdgeFlagPointer(DrawItemContext &aContext)
{
	GLsizei stride(Expression::Read<GLsizei>(aContext));
	size_t count(Expression::Read<size_t>(aContext));
	glEdgeFlagPointer(stride, aContext.mStream);
	aContext.mStream += (count*sizeof(bool)+sizeof(unsigned int)-1)/sizeof(unsigned int);
}

void DO_glEnable(DrawItemContext &aContext)
{
	glEnable(Expression::Read<GLenum>(aContext));
}

void DO_glEnableClientState(DrawItemContext &aContext)
{
	glEnableClientState(Expression::Read<GLenum>(aContext));
}

void DO_glEnd(DrawItemContext &aContext)
{
	glEnd();
}

void DO_glIndexf(DrawItemContext &aContext)
{
	float value(Expression::Evaluate<DLIndex>(aContext));
	glIndexf(value);
}

void DO_glIndexPointer(DrawItemContext &aContext)
{
	GLsizei stride(Expression::Read<GLsizei>(aContext));
	size_t count(Expression::Read<size_t>(aContext));
	glIndexPointer(GL_FLOAT, stride, aContext.mStream);
	aContext.mStream += (count*sizeof(GLfloat)+sizeof(unsigned int)-1)/sizeof(unsigned int);
}

void DO_glLoadIdentity(DrawItemContext &aContext)
{
	glLoadIdentity();
}

void DO_glLoadMatrixf(DrawItemContext &aContext)
{
	glLoadMatrixf(reinterpret_cast<const GLfloat *>(aContext.mStream));
	aContext.mStream += (16*sizeof(GLfloat)+sizeof(unsigned int)-1)/sizeof(unsigned int);
}

void DO_glMultMatrixf(DrawItemContext &aContext)
{
	glMultMatrixf(reinterpret_cast<const GLfloat *>(aContext.mStream));
	aContext.mStream += (16*sizeof(GLfloat)+sizeof(unsigned int)-1)/sizeof(unsigned int);
}

void DO_glNormal3fv(DrawItemContext &aContext)
{
	DLNormal value(Expression::Evaluate<DLNormal>(aContext));
	glNormal3fv(reinterpret_cast<float *>(&value));
}

void DO_glNormalPointer(DrawItemContext &aContext)
{
	GLsizei stride(Expression::Read<GLsizei>(aContext));
	size_t count(Expression::Read<size_t>(aContext));
	glNormalPointer(GL_FLOAT, stride, aContext.mStream);
	aContext.mStream += (count*sizeof(GLfloat)+sizeof(unsigned int)-1)/sizeof(unsigned int);
}

void DO_glPopAttrib(DrawItemContext &aContext)
{
	glPopAttrib();
}

void DO_glPopClientAttrib(DrawItemContext &aContext)
{
	glPopClientAttrib();
}

void DO_glPopMatrix(DrawItemContext &aContext)
{
	glPopMatrix();
}

void DO_glPushAttrib(DrawItemContext &aContext)
{
	glPushAttrib(Expression::Read<GLbitfield>(aContext));
}

void DO_glPushClientAttrib(DrawItemContext &aContext)
{
	glPushClientAttrib(Expression::Read<GLbitfield>(aContext));
}

void DO_glPushMatrix(DrawItemContext &aContext)
{
	glPushMatrix();
}

void DO_glRotatef(DrawItemContext &aContext)
{
	float value(Expression::Evaluate<DLRotation>(aContext));
	glRotatef(value, 0, 0, 1);
}

void DO_glScalef(DrawItemContext &aContext)
{
	DLScale value(Expression::Evaluate<DLScale>(aContext));
	glScalef(reinterpret_cast<float *>(&value)[0], reinterpret_cast<float *>(&value)[1], reinterpret_cast<float *>(&value)[2]);
}

void DO_glTexCoord2fv(DrawItemContext &aContext)
{
	DLTexCoord value(Expression::Evaluate<DLTexCoord>(aContext));
	glTexCoord2fv(reinterpret_cast<float *>(&value));
}

void DO_glTexCoordPointer(DrawItemContext &aContext)
{
	GLint size(Expression::Read<GLint>(aContext));
	GLsizei stride(Expression::Read<GLsizei>(aContext));
	size_t count(Expression::Read<size_t>(aContext));
	glTexCoordPointer(size, GL_FLOAT, stride, aContext.mStream);
	aContext.mStream += (count*sizeof(GLfloat)+sizeof(unsigned int)-1)/sizeof(unsigned int);
}

void DO_glTranslatef(DrawItemContext &aContext)
{
	DLTranslation value(Expression::Evaluate<DLTranslation>(aContext));
	glTranslatef(reinterpret_cast<float *>(&value)[0], reinterpret_cast<float *>(&value)[1], reinterpret_cast<float *>(&value)[2]);
}

void DO_glVertex3fv(DrawItemContext &aContext)
{
	DLPosition value(Expression::Evaluate<DLPosition>(aContext));
	glVertex3fv(reinterpret_cast<float *>(&value));
}

void DO_glVertexPointer(DrawItemContext &aContext)
{
	GLint size(Expression::Read<GLint>(aContext));
	GLsizei stride(Expression::Read<GLsizei>(aContext));
	size_t count(Expression::Read<size_t>(aContext));
	glVertexPointer(size, GL_FLOAT, stride, aContext.mStream);
	aContext.mStream += (count*sizeof(GLfloat)+sizeof(unsigned int)-1)/sizeof(unsigned int);
}

void DO_Repeat(DrawItemContext &aContext)
{
	int repeat(Expression::Read<int>(aContext));
	size_t size(Expression::Read<size_t>(aContext));
	DrawItemContext context(aContext);
	context.mEnd = context.mStream + size;
	for (int i = 0; i < repeat; i++)
	{
		context.mStream = aContext.mStream;
		ExecuteDrawItems(context);
	}
	aContext.mStream += size;
}

void DO_Block(DrawItemContext &aContext)
{
	float start(Expression::Read<float>(aContext));
	float length(Expression::Read<float>(aContext));
	float scale(Expression::Read<float>(aContext));
	int repeat(Expression::Read<int>(aContext));
	size_t size(Expression::Read<size_t>(aContext));
	float t = aContext.mParam - start;
	if (t >= 0.0f && length > 0.0f)
	{
		int loop = xs_FloorToInt(t / length);
		if (repeat < 0 || loop <= repeat)
		{
			t -= loop * length;
			t *= scale;
			DrawItemContext context(aContext);
			context.mEnd = context.mStream + size;
			context.mParam = t;
			ExecuteDrawItems(context);
		}
	}
	aContext.mStream += size;
}

void DO_Set(DrawItemContext &aContext)
{
	EvaluateVariableOperator(aContext, VariableOperatorSet);
}

void DO_Add(DrawItemContext &aContext)
{
	EvaluateVariableOperator(aContext, VariableOperatorAdd);
}

void DO_Sub(DrawItemContext &aContext)
{
	EvaluateVariableOperator(aContext, VariableOperatorSub);
}

void DO_Mul(DrawItemContext &aContext)
{
	EvaluateVariableOperator(aContext, VariableOperatorMul);
}

void DO_Div(DrawItemContext &aContext)
{
	EvaluateVariableOperator(aContext, VariableOperatorDiv);
}

void DO_Min(DrawItemContext &aContext)
{
	EvaluateVariableOperator(aContext, VariableOperatorMin);
}

void DO_Max(DrawItemContext &aContext)
{
	EvaluateVariableOperator(aContext, VariableOperatorMax);
}

void DO_Swizzle(DrawItemContext &aContext)
{
	unsigned int name(Expression::Read<unsigned int>(aContext));
	int width(Expression::Read<int>(aContext));
//	Database::Typed<float> &variables = Database::variable.Open(aContext.mId);
	float *temp = static_cast<float *>(_alloca(width * sizeof(float)));
	for (int i = 0; i < width; i++)
		temp[i] = aContext.mVars->Get(name + Expression::Read<unsigned int>(aContext));
	for (int i = 0; i < width; i++)
		aContext.mVars->Put(name + i, temp[i]);
//	Database::variable.Close(aContext.mId);
}

void DO_Clear(DrawItemContext &aContext)
{
	unsigned int name(Expression::Read<unsigned int>(aContext));
	int width(Expression::Read<int>(aContext));
//	Database::Typed<float> &variables = Database::variable.Open(aContext.mId);
	for (int i = 0; i < width; i++)
		aContext.mVars->Delete(name+i);
//	Database::variable.Close(aContext.mId);
}

#ifdef DRAWLIST_LOOP
void DO_Loop(DrawItemContext &aContext)
{
	unsigned int name = Expression::Read<unsigned int>(aContext);
	float from = Expression::Read<float>(aContext);
	float to   = Expression::Read<float>(aContext);
	float by   = Expression::Read<float>(aContext);
	size_t size = Expression::Read<size_t>(aContext);

//		Database::Typed<float> &variables = Database::variable.Open(aContext.mId);
	DrawItemContext context(aContext);
	context.mEnd = context.mStream + size;
	if (by > 0)
	{
		for (float value = from; value <= to; value += by)
		{
			context.mVars->Put(name, value);
			context.mStream = aContext.mStream;
			ExecuteDrawItems(context);
		}
	}
	else
	{
		for (float value = from; value >= to; value += by)
		{
			context.mVars->Put(name, value);
			context.mStream = aContext.mStream;
			ExecuteDrawItems(context);
		}
	}
	context.mVars->Delete(name);
//		Database::variable.Close(aContext.mId);

	aContext.mStream += size;
}
#endif


//
// LITERAL EXPRESSION
// returns an embedded constant value
//

// float[width] literal
void ConfigureLiteral(const TiXmlElement *element, std::vector<unsigned int> &buffer, int width, const char * const names[], const float defaults[])
{
	// process literal data
	for (int i = 0; i < width; ++i)
	{
		float value = defaults[i];
		element->QueryFloatAttribute(names[i], &value);
		buffer.push_back(*reinterpret_cast<unsigned int *>(&value));
		DebugPrint(" %s=%f", names[i], value);
	}
}

// typed literal
template <typename T> void ConfigureLiteral(const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[])
{
	// append a constant expression
	DebugPrint("%s literal:", Expression::Schema<T>::NAME);
	Expression::Append(buffer, Expression::Constant<T>);
	ConfigureLiteral(element, buffer, sizeof(T)/sizeof(float), names, defaults);
	DebugPrint("\n");
}
/*
template <> void ConfigureLiteral<__m128>(const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[])
{
	Expression::Append(buffer, Expression::Constant<__m128>);
	__m128 value = _mm_setzero_ps();
	for (int i = 0; i < 4; ++i)
	{
		reinterpret_cast<float *>(&value)[i] = defaults[i];
		element->QueryFloatAttribute(names[i], &reinterpret_cast<float *>(&value)[i]);
	}
	Expression::New<__m128>(buffer, value);
}
*/

//
// VARIABLE EXPRESSION
// returns the value of a named variable
//

// evaluate float[width] variable
void EvaluateVariable(float value[], int width, DrawItemContext &aContext)
{
	unsigned int name = Expression::Read<unsigned int>(aContext);
//	const Database::Typed<float> &variables = Database::variable.Get(aContext.mId);
	for (int i = 0; i < width; ++i)
		value[i] = aContext.mVars->Get(name+i);
}

// evaluate typed variable
template <typename T> static const T EvaluateVariable(DrawItemContext &aContext)
{
	T value = T();
	EvaluateVariable(reinterpret_cast<float * __restrict>(&value), sizeof(T)/sizeof(float), aContext);
	return value;
}

// typed variable: attribute-inlined version
template <typename T> void ConfigureInlineVariable(const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[])
{
	// append a variable expression
	DebugPrint("%s variable %s (inline)\n", Expression::Schema<T>::NAME, element->Attribute("variable"));
	Expression::Append(buffer, EvaluateVariable<T>, Hash(element->Attribute("variable")));
}

// typed variable: normal version
template <typename T> void ConfigureVariable(const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[])
{
	// append a variable expression
	DebugPrint("%s variable %s\n", Expression::Schema<T>::NAME, element->Attribute("name"));
	Expression::Append(buffer, EvaluateVariable<T>, Hash(element->Attribute("name")));
}

// typed variable: tag-named version
template <typename T> void ConfigureTagVariable(const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[])
{
	// append a variable expression
	DebugPrint("%s variable %s (tag)\n", Expression::Schema<T>::NAME, element->Value());
	Expression::Append(buffer, EvaluateVariable<T>, Hash(element->Value()));
}


//
// INTERPOLATOR EXPRESSION
// returns interpolated value based on parameter
//

#pragma optimize( "t", on )

// evaluate float[width] interpolator
void EvaluateInterpolator(float value[], int width, float param, DrawItemContext &aContext)
{
	// data size
	unsigned int size = Expression::Read<unsigned int>(aContext);

	// end of data
	const unsigned int *end = aContext.mStream + size;

	// get interpolator value
	const int count = Expression::Read<int>(aContext);
	const float * __restrict keys = reinterpret_cast<const float * __restrict>(aContext.mStream);
	int dummy = 0;
	if (!ApplyInterpolator(value, width, count, keys, param, dummy))
		memset(value, 0, width * sizeof(float));

	// advance stream
	aContext.mStream = end;
}

// evaluate typed interpolator
template <typename T> static const T EvaluateInterpolator(DrawItemContext &aContext)
{
	T value = T();
	float param(Expression::Evaluate<float>(aContext));
	EvaluateInterpolator(reinterpret_cast<float * __restrict>(&value), sizeof(T)/sizeof(float), param, aContext);
	return value;
}

template <typename T> T ApplyInterpolator(int aCount, const float aKeys[], float aTime, int &aHint);

template<> float ApplyInterpolator<float>(int aCount, const float aKeys[], float aTime, int &aHint)
{
	// get stride
	const int aStride = sizeof(float)/sizeof(float) + 1;

	// find the key index
	int index = FindKeyIndex(aStride, aCount, aKeys, aTime, aHint);
	if (index < 0)
		return 0.0f;

	// interpolate the value
	const float * __restrict key = aKeys + index * aStride;
	const float time0 = key[0];
	const float data0 = key[1];
	const float time1 = key[aStride];
	const float data1 = key[aStride + 1];
	const float t = (aTime - time0) / (time1 - time0 + FLT_EPSILON);
	return data0 + (data1 - data0) * t;
}

template <> __m128 ApplyInterpolator<__m128>(int aCount, const float aKeys[], float aTime, int &aHint)
{
	// get stride
	const int aStride = sizeof(__m128)/sizeof(float) + 1;

	// find the key index
	int index = FindKeyIndex(aStride, aCount, aKeys, aTime, aHint);
	if (index < 0)
		return _mm_setzero_ps();

	// interpolate the value
	const float * __restrict key = aKeys + index * aStride;
	const float time0 = key[0];
	const __m128 data0 = _mm_loadu_ps(&key[1]);
	const float time1 = key[aStride];
	const __m128 data1 = _mm_loadu_ps(&key[aStride + 1]);
	const float t = (aTime - time0) / (time1 - time0 + FLT_EPSILON);
	return _mm_add_ps(data0, _mm_mul_ps(_mm_sub_ps(data1, data0), _mm_set_ps1(t)));
}

template <> static const float EvaluateInterpolator<float>(DrawItemContext &aContext)
{
	//bool ApplyInterpolator(float aTarget[], int aWidth, int aCount, const float aKeys[], float aTime, int &aHint)

	// get parameter value
	float aTime(Expression::Evaluate<float>(aContext));

	// data size
	unsigned int size = Expression::Read<unsigned int>(aContext);

	// end of data
	const unsigned int *end = aContext.mStream + size;

	// get interpolator data
	const int aCount = Expression::Read<int>(aContext);
	const float * __restrict aKeys = reinterpret_cast<const float * __restrict>(aContext.mStream);
	int aHint = 0;

	// get interpolated value
	float value = ApplyInterpolator<float>(aCount, aKeys, aTime, aHint);

	// advance stream
	aContext.mStream = end;

	// return value
	return value;
}

template <> static const __m128 EvaluateInterpolator<__m128>(DrawItemContext &aContext)
{
	//bool ApplyInterpolator(float aTarget[], int aWidth, int aCount, const float aKeys[], float aTime, int &aHint)

	// get parameter value
	float aTime(Expression::Evaluate<float>(aContext));

	// data size
	unsigned int size = Expression::Read<unsigned int>(aContext);

	// end of data
	const unsigned int *end = aContext.mStream + size;

	// get interpolator data
	const int aCount = Expression::Read<int>(aContext);
	const float * __restrict aKeys = reinterpret_cast<const float * __restrict>(aContext.mStream);
	int aHint = 0;

	// get interpolated value
	__m128 value = ApplyInterpolator<__m128>(aCount, aKeys, aTime, aHint);

	// advance stream
	aContext.mStream = end;

	// return value
	return value;
}

#pragma optimize( "", on )


// configure float[width] interpolator
static void ConfigureInterpolator(const TiXmlElement *element, std::vector<unsigned int> &buffer, int width, const char * const names[], const float defaults[])
{
	// process interpolator data
	buffer.push_back(0);
	int start = buffer.size();
	ConfigureInterpolatorItem(element, buffer, width, names, defaults);
	buffer[start - 1] = buffer.size() - start;
}

// configure typed interpolator
template <typename T> void ConfigureInterpolator(const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[])
{
	// append an interpolator expression
	DebugPrint("%s interpolator\n", Expression::Schema<T>::NAME);
	Expression::Append(buffer, EvaluateInterpolator<T>);
	if (const TiXmlElement *param = element->FirstChildElement("param"))
		ConfigureExpressionRoot<float>(param, buffer, sScalarNames, sScalarDefault);
	else
		Expression::Append(buffer, EvaluateTime);
	ConfigureInterpolator(element, buffer, sizeof(T)/sizeof(float), names, defaults);
}


//
// RANDOM EXPRESSION
// returns a random value
//

// TO DO: float[width] random

// configure typed random
template <typename T> void ConfigureRandom(const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[])
{
	// width in floats (HACK)
	const int width = (sizeof(T)+sizeof(float)-1)/sizeof(float);

	// get random count
	int count = 1;
	element->QueryIntAttribute("rand", &count);

	DebugPrint("%s random %d:", Expression::Schema<T>::NAME, count);

	// offset factor
	float offset[width];

	// scale factor
	float scale[width];

	// status flags
	bool need_offset = false;
	bool need_scale = false;
	bool need_rand = false;

	// get component properties
	for (int i = 0; i < width; i++)
	{
		char label[64];

		// default values
		offset[i] = defaults[i];
		scale[i] = 0;

		// average
		sprintf(label, "%s_avg", names[i]);
		float average = defaults[i];
		if (element->QueryFloatAttribute(label, &average) == TIXML_SUCCESS)
		{
			offset[i] = average;
		}

		// variance
		sprintf(label, "%s_var", names[i]);
		float variance = 0.0f;
		if (element->QueryFloatAttribute(label, &variance) == TIXML_SUCCESS)
		{
			offset[i] = average - variance;
			scale[i] = variance * 2;
		}

		// minimum
		sprintf(label, "%s_min", names[i]);
		float minimum = 0.0f;
		if (element->QueryFloatAttribute(label, &minimum) == TIXML_SUCCESS)
		{
			offset[i] = minimum;
		}

		// maximum
		sprintf(label, "%s_max", names[i]);
		float maximum = 0.0f;
		if (element->QueryFloatAttribute(label, &maximum) == TIXML_SUCCESS)
		{
			scale[i] = maximum - minimum;
		}

		// update status
		if (offset[i] != 0.0f)
			need_offset = true;

		if (count > 0)
		{
			// compensate for random count
			scale[i] /= count;

			if (scale[i] != 0.0f)
				need_rand = true;
			if (scale[i] != 1.0f)
				need_scale = true;
		}

		DebugPrint(" %f+%f*%s", offset[i], scale[i], names[i]);
	}

	if (need_offset || !need_rand)
	{
		if (need_rand)
		{
			// push add
			Expression::Append(buffer, Expression::Add<T>);
		}

		// push offset
		Expression::Append(buffer, Expression::Constant<T>);
		for (int i = 0; i < width; ++i)
			Expression::New<float>(buffer, offset[i]);
	}

	if (need_rand)
	{
		if (need_scale)
		{
			// push multiply
			Expression::Append(buffer, Expression::Mul<T>);

			// push scale
			Expression::Append(buffer, Expression::Constant<T>);
			for (int i = 0; i < width; ++i)
				Expression::New<float>(buffer, scale[i]);
		}

		for (int i = 0; i < count; ++i)
		{
			if (count > 1 && i < count - 1)
			{
				// push add
				Expression::Append(buffer, Expression::Add<T>);
			}

			// push randoms
			Expression::Append(buffer, Expression::ComponentNullary<T, Expression::Schema<T>::COUNT>::Evaluate<float, Random::Float>);
		}
	}

	DebugPrint("\n");
}


//
// NOISE EXPRESSION
//
template <typename T> void ConfigureNoise(const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[])
{
	const TiXmlElement *arg1 = element->FirstChildElement();
	if (!arg1)
	{
		DebugPrint("%s noise ->", Expression::Schema<T>::NAME);
		ConfigureLiteral<T>(element, buffer, names, defaults);
		return;
	}

	const TiXmlElement *arg2 = arg1->NextSiblingElement();
	if (!arg2)
	{
		DebugPrint("%s noise1\n", Expression::Schema<T>::NAME);
		Expression::Append(buffer, Expression::ComponentUnary<T, Expression::Schema<T>::COUNT>::Evaluate<float, float, Noise>);
		ConfigureExpression<T>(arg1, buffer, names, defaults);
		return;
	}

	const TiXmlElement *arg3 = arg2->NextSiblingElement();
	if (!arg3)
	{
		DebugPrint("%s noise2\n", Expression::Schema<T>::NAME);
		Expression::Append(buffer, Expression::ComponentBinary<T, Expression::Schema<T>::COUNT>::Evaluate<float, float, float, Noise>);
		ConfigureExpression<T>(arg1, buffer, names, defaults);
		ConfigureExpression<T>(arg2, buffer, names, defaults);
		return;
	}

	const TiXmlElement *arg4 = arg3->NextSiblingElement();
	if (!arg4)
	{
		DebugPrint("%s noise3\n", Expression::Schema<T>::NAME);
		Expression::Append(buffer, Expression::ComponentTernary<T, Expression::Schema<T>::COUNT>::Evaluate<float, float, float, float, Noise>);
		ConfigureExpression<T>(arg1, buffer, names, defaults);
		ConfigureExpression<T>(arg2, buffer, names, defaults);
		ConfigureExpression<T>(arg3, buffer, names, defaults);
		return;
	}

	DebugPrint("%s noise4\n", Expression::Schema<T>::NAME);
	assert(false);
}


//
// TIME EXPRESSION
//

float EvaluateTime(DrawItemContext &aContext)
{
	return aContext.mParam;
}


//
// OSCILLATOR EXPRESSION
//

template <typename T> void ConfigureSineWave(const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[])
{
	// add offset
	if (const TiXmlElement *child = element->FirstChildElement("offset"))
	{
		Expression::Append(buffer, Expression::Add<T>);
		ConfigureExpressionRoot<T>(child, buffer, names, defaults);
	}

	// apply scale
	if (const TiXmlElement *child = element->FirstChildElement("scale"))
	{
		Expression::Append(buffer, Expression::Mul<T>);
		ConfigureExpressionRoot<T>(child, buffer, names, sScaleDefault);
	}

	// extend float -> T
	Expression::Append(buffer, Expression::Extend<T, float>);

	// sine
	Expression::Append(buffer, Expression::Sin<float>);

	// scale by 2 PI
	Expression::Append(buffer, Expression::Mul<float>);
	Expression::Append(buffer, Expression::Constant<float>, float(M_PI)*2.0f);

	// add phase
	float phase = 0.0f;
	if (const TiXmlElement *child = element->FirstChildElement("phase"))
	{
		Expression::Append(buffer, Expression::Add<float>);
		ConfigureExpressionRoot<float>(child, buffer, sScalarNames, &phase);
	}
	else
	{
		element->QueryFloatAttribute("phase", &phase);
		if (phase != 0.0f)
		{
			Expression::Append(buffer, Expression::Add<float>);
			Expression::Append(buffer, Expression::Constant<float>, phase);
		}
	}

	// apply period
	float period = 1.0f;
	if (const TiXmlElement *child = element->FirstChildElement("period"))
	{
		Expression::Append(buffer, Expression::Mul<float>);
		Expression::Append(buffer, Expression::Rcp<float>);
		ConfigureExpressionRoot<float>(child, buffer, sScalarNames, &period);
	}
	else
	{
		element->QueryFloatAttribute("period", &period);
		if (period != 1.0f)
		{
			Expression::Append(buffer, Expression::Mul<float>);
			Expression::Append(buffer, Expression::Constant<float>, 1.0f/period);
		}
	}

	// apply frequency
	float frequency = 1.0f;
	if (const TiXmlElement *child = element->FirstChildElement("frequency"))
	{
		Expression::Append(buffer, Expression::Mul<float>);
		ConfigureExpressionRoot<float>(child, buffer, sScalarNames, &frequency);
	}
	else
	{
		element->QueryFloatAttribute("frequency", &frequency);
		if (frequency != 1.0f)
		{
			Expression::Append(buffer, Expression::Mul<float>);
			Expression::Append(buffer, Expression::Constant<float>, frequency);
		}
	}

	// get input
	if (const TiXmlElement *child = element->FirstChildElement("input"))
	{
		ConfigureExpressionRoot<float>(child, buffer, sScalarNames, sScalarDefault);
	}
	else
	{
		Expression::Append(buffer, EvaluateTime);
	}
}
template <typename T> void ConfigureTriangleWave(const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[])
{
}
template <typename T> void ConfigureSquareWave(const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[])
{
}
template <typename T> void ConfigureSawtoothWave(const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[])
{
}


//
// TYPE CONVERSION
//
template <typename T, typename A> struct Convert
{
	static void Append(std::vector<unsigned int> &buffer)
	{
		Expression::Append(buffer, Expression::Construct<T, A>);
	}
};
template <typename T> struct Convert<T, T>
{
	static void Append(std::vector<unsigned int> &buffer)
	{
	}
};
template <> struct Convert<Vector2, float>
{
	static void Append(std::vector<unsigned int> &buffer)
	{
		Expression::Append(buffer, Expression::Extend<Vector2, float>);
	}
};
template <> struct Convert<Vector3, float>
{
	static void Append(std::vector<unsigned int> &buffer)
	{
		Expression::Append(buffer, Expression::Extend<Vector3, float>);
	}
};
template <> struct Convert<Vector4, float>
{
	static void Append(std::vector<unsigned int> &buffer)
	{
		Expression::Append(buffer, Expression::Extend<Vector4, float>);
	}
};
template <> struct Convert<Color4, float>
{
	static void Append(std::vector<unsigned int> &buffer)
	{
		Expression::Append(buffer, Expression::Extend<Color4, float>);
	}
};
template <> struct Convert<__m128, float>
{
	static void Append(std::vector<unsigned int> &buffer)
	{
		Expression::Append(buffer, Expression::Extend<__m128, float>);
	}
};
template <typename T, typename A> void ConfigureConvert(const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[])
{
	DebugPrint("%s convert %s\n", Expression::Schema<T>::NAME, Expression::Schema<A>::NAME);

	const TiXmlElement *arg1 = element->FirstChildElement();
	if (!arg1)
	{
		// no first argument: treat element as a literal (HACK)
		assert(!"no argument for type conversion");
		ConfigureLiteral<T>(element, buffer, names, defaults);
		return;
	}

	// append the operator
	Convert<T, A>::Append(buffer);

	// append first argument
	ConfigureExpression<A>(arg1, buffer, names, defaults);
}

//
// CONSTRUCT EXPRESSION
//
template <typename T> void ConfigureConstruct(const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[])
{
	DebugPrint("%s construct\n", Expression::Schema<T>::NAME);

	// width in floats (HACK)
	const int width = (sizeof(T)+sizeof(float)-1)/sizeof(float);

	// append the operator
	Expression::Append(buffer, Expression::Construct<T>);

	// for each component...
	for (int i = 0; i < width; ++i)
	{
		// if there is a corresponding tag...
		if (const TiXmlElement *component = element->FirstChildElement(names[i]))
		{
			// configure the expression
			ConfigureExpressionRoot<DLScalar>(component, buffer, sScalarNames, &defaults[i]);
		}
		else
		{
			// use default value
			DebugPrint("%s default %s: %f\n", Expression::Schema<float>::NAME, names[i], defaults[i]);
			Expression::Append(buffer, Expression::Constant<float>, defaults[i]);
		}
	}
}


//
// UNARY EXPRESSION
// return the result of an expression taking one parameter
//
template <typename T, typename A, typename C> void ConfigureUnary(T (expr)(C), const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[])
{
	const TiXmlElement *arg1 = element->FirstChildElement();
	if (!arg1)
	{
		// no first argument: treat element as a literal (HACK)
		assert(!"no argument for unary operator");
		ConfigureLiteral<T>(element, buffer, names, defaults);
		return;
	}

	// append the operator
	Expression::Append(buffer, expr);

	// append first argument
	ConfigureExpression<A>(arg1, buffer, names, defaults);
}


//
// BINARY EXPRESSION
// return the result of an expression taking two parameters
//
template <typename T, typename A1, typename A2, typename C> void ConfigureBinary(T (expr)(C), const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[])
{
	const TiXmlElement *arg1 = element->FirstChildElement();
	if (!arg1)
	{
		// no first argument: treat element as a literal (HACK)
		assert(!"no first argument for binary operator");
		ConfigureLiteral<T>(element, buffer, names, defaults);
		return;
	}

	const TiXmlElement *arg2 = arg1->NextSiblingElement();
	if (!arg2)
	{
		// no second argument: treat element as a literal (HACK)
		assert(!"no second argument for binary operator");
		ConfigureLiteral<T>(element, buffer, names, defaults);
		return;
	}

	// append the operator
	Expression::Append(buffer, expr);

	// append first argument
	ConfigureExpression<A1>(arg1, buffer, names, defaults);

	// append second argument
	ConfigureExpression<A2>(arg2, buffer, names, defaults);
}


//
// TERNARY EXPRESSION
// return the result of an expression taking three parameters
//
template <typename T, typename A1, typename A2, typename A3, typename C> void ConfigureTernary(T (expr)(C), const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[])
{
	const TiXmlElement *arg1 = element->FirstChildElement();
	if (!arg1)
	{
		// no first argument: treat element as a literal (HACK)
		assert(!"no first argument for ternary operator");
		ConfigureLiteral<T>(element, buffer, names, defaults);
		return;
	}

	const TiXmlElement *arg2 = arg1->NextSiblingElement();
	if (!arg2)
	{
		// no second argument: treat element as a literal (HACK)
		assert(!"no second argument for ternary operator");
		ConfigureLiteral<T>(element, buffer, names, defaults);
		return;
	}

	const TiXmlElement *arg3 = arg2->NextSiblingElement();
	if (!arg3)
	{
		// no third argument: treat element as a literal (HACK)
		assert(!"no third argument for ternary operator");
		ConfigureLiteral<T>(element, buffer, names, defaults);
		return;
	}

	// append the operator
	Expression::Append(buffer, expr);

	// append first argument
	ConfigureExpression<A1>(arg1, buffer, names, defaults);

	// append second argument
	ConfigureExpression<A2>(arg2, buffer, names, defaults);

	// append third argument
	ConfigureExpression<A3>(arg3, buffer, names, defaults);
}


//
// VARIADIC EXPRESSION
// binary-to-variadic adapter
//

// configure typed variadic
template <typename T, typename A, typename C> void ConfigureVariadic(T (expr)(C), const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[])
{
	const TiXmlElement *arg1 = element->FirstChildElement();
	if (!arg1)
	{
		// no first argument: treat element as a literal (HACK)
		DebugPrint("no first argument for variadic operator");
		ConfigureLiteral<T>(element, buffer, names, defaults);
		return;
	}

	const TiXmlElement *arg2 = arg1->NextSiblingElement();
	if (!arg2)
	{
		// no second argument: convert type of first argument (HACK)
		DebugPrint("no second argument for variadic operator: performing type conversion");
		Convert<T, A>::Append(buffer);
		ConfigureExpression<A>(arg1, buffer, names, defaults);
		return;
	}

	// rewind
	arg2 = arg1;
	do
	{
		// get next argument
		arg1 = arg2;
		arg2 = arg2->NextSiblingElement();

		// if there is a second argument...
		if (arg2)
		{
			// append the operator
			Expression::Append(buffer, expr);
		}

		// append first argument
		ConfigureExpression<A>(arg1, buffer, names, defaults);
	}
	while (arg2);
}


//
// EXPRESSION
//

// configure an expression
template <typename T> void ConfigureExpression(const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[])
{
	DebugPrint("%s expression %s\n", Expression::Schema<T>::NAME, element->Value());

	// width in floats (HACK)
	const int width = (sizeof(T)+sizeof(float)-1)/sizeof(float);

	// copy defaults
	float *data = static_cast<float *>(_alloca(width * sizeof(float)));
	memcpy(data, defaults, width * sizeof(float));

	// get hash of tag name
	unsigned int hash = Hash(element->Value());

	// read literal values from attributes (if any)
	bool overrided = false;
	for (int i = 0; i < width; ++i)
	{
		if (element->QueryFloatAttribute(names[i], &data[i]) == TIXML_SUCCESS)
			overrided = true;
	}


	// configure based on tag name
	switch(hash)
	{
	case 0x425ed3ca /* "value" */:			ConfigureLiteral<T>(element, buffer, names, data); return;
	case 0x19385305 /* "variable" */:		ConfigureVariable<T>(element, buffer, names, data); return;
	case 0x83588fd4 /* "interpolator" */:	ConfigureInterpolator<T>(element, buffer, names, data); return;
	case 0xa19b8cd6 /* "rand" */:			ConfigureRandom<T>(element, buffer, names, data); return;
	case 0x904416d1 /* "noise" */:			ConfigureNoise<T>(element, buffer, names, data); return;

	case 0xaa7d7949 /* "extend" */:			ConfigureUnary<const T, float, Expression::Context &>(Expression::Extend<T, float>, element, buffer, sScalarNames, sScalarDefault); return;
	case 0x40c09172 /* "construct" */:		ConfigureConstruct<T>(element, buffer, names, data); return;
	case 0x5d3c9be4 /* "time" */:			Expression::Append(buffer, EvaluateTime); return;

	case 0x3b391274 /* "add" */:			ConfigureVariadic<T, T>(Expression::Add<T>, element, buffer, names, data); return;
	case 0xdc4e3915 /* "sub" */:			ConfigureVariadic<T, T>(Expression::Sub<T>, element, buffer, names, data); return;
	case 0xeb84ed81 /* "mul" */:			ConfigureVariadic<T, T>(Expression::Mul<T>, element, buffer, names, data); return;
	case 0xe562ab48 /* "div" */:			ConfigureVariadic<T, T>(Expression::Div<T>, element, buffer, names, data); return;
	case 0x3899af41 /* "neg" */:			ConfigureUnary<T, T>(Expression::Neg<T>, element, buffer, names, data); return;
	case 0x31037236 /* "rcp" */:			ConfigureUnary<T, T>(Expression::Rcp<T>, element, buffer, names, data); return;
	case 0xa8e99c47 /* "inc" */:			ConfigureUnary<T, T>(Expression::Inc<T>, element, buffer, names, data); return;
	case 0xc25979d3 /* "dec" */:			ConfigureUnary<T, T>(Expression::Dec<T>, element, buffer, names, data); return;
									
	case 0xe0302a4d /* "sin" */:			ConfigureUnary<T, T>(Expression::Sin<T>, element, buffer, names, data); return;
	case 0xfb8de29c /* "cos" */:			ConfigureUnary<T, T>(Expression::Cos<T>, element, buffer, names, data); return;
	case 0x9cf73498 /* "tan" */:			ConfigureUnary<T, T>(Expression::Tan<T>, element, buffer, names, data); return;
	case 0xfeae7ea6 /* "asin" */:			ConfigureUnary<T, T>(Expression::Asin<T>, element, buffer, names, data); return;
	case 0x3c01df1f /* "acos" */:			ConfigureUnary<T, T>(Expression::Acos<T>, element, buffer, names, data); return;
	case 0x0678cabf /* "atan" */:			ConfigureUnary<T, T>(Expression::Atan<T>, element, buffer, names, data); return;
	case 0xbd26dbf7 /* "atan2" */:			ConfigureBinary<T, T, T>(Expression::Atan2<T>, element, buffer, names, data); return;
									
	case 0x10d2583f /* "sinh" */:			ConfigureUnary<T, T>(Expression::Sinh<T>, element, buffer, names, data); return;
	case 0xf45c461c /* "cosh" */:			ConfigureUnary<T, T>(Expression::Cosh<T>, element, buffer, names, data); return;
	case 0x092855d0 /* "tanh" */:			ConfigureUnary<T, T>(Expression::Tanh<T>, element, buffer, names, data); return;
									
	case 0x58336ad5 /* "pow" */:			ConfigureBinary<T, T, T>(Expression::Pow<T>, element, buffer, names, data); return;
	case 0x72a68728 /* "exp" */:			ConfigureUnary<T, T>(Expression::Exp<T>, element, buffer, names, data); return;
	case 0x3f515151 /* "log" */:			ConfigureUnary<T, T>(Expression::Log<T>, element, buffer, names, data); return;
	case 0x7dee3bcf /* "sqrt" */:			ConfigureUnary<T, T>(Expression::Sqrt<T>, element, buffer, names, data); return;
	case 0x0a6a5946 /* "invsqrt" */:		ConfigureUnary<T, T>(Expression::InvSqrt<T>, element, buffer, names, data); return;
									
	case 0x2a48023b /* "abs" */:			ConfigureUnary<T, T>(Expression::Abs<T>, element, buffer, names, data); return;
	case 0x0cbc8ba4 /* "sign" */:			ConfigureUnary<T, T>(Expression::Sign<T>, element, buffer, names, data); return;
	case 0xb8e70c1d /* "floor" */:			ConfigureUnary<T, T>(Expression::Floor<T>, element, buffer, names, data); return;
	case 0x62e4e208 /* "ceil" */:			ConfigureUnary<T, T>(Expression::Ceil<T>, element, buffer, names, data); return;
	case 0x87aad829 /* "frac" */:			ConfigureUnary<T, T>(Expression::Frac<T>, element, buffer, names, data); return;
	case 0xdf9e7283 /* "mod" */:			ConfigureBinary<T, T, T>(Expression::Mod<T>, element, buffer, names, data); return;
	case 0xc98f4557 /* "min" */:			ConfigureVariadic<T, T>(Expression::Min<T>, element, buffer, names, data); return;
	case 0xd7a2e319 /* "max" */:			ConfigureVariadic<T, T>(Expression::Max<T>, element, buffer, names, data); return;
	case 0xa82efcbc /* "clamp" */:			ConfigureTernary<T, T, T, T>(Expression::Clamp<T>, element, buffer, names, data); return;
	case 0x1e691468 /* "lerp" */:			ConfigureTernary<T, T, T, float>(Expression::Lerp<T>, element, buffer, names, data); return;
	case 0xc7441a0f /* "step" */:			ConfigureBinary<T, T, T>(Expression::Step<T>, element, buffer, names, data); return;
	case 0x95964e7d /* "smoothstep" */:		ConfigureTernary<T, T, T, T>(Expression::SmoothStep<T>, element, buffer, names, data); return;

	case 0xb711f539 /* "sinewave" */:		ConfigureSineWave<T>(element, buffer, names, data); return;
	case 0xd0308494 /* "trianglewave" */:	ConfigureTriangleWave<T>(element, buffer, names, data); return;
	case 0x3d18154f /* "squarewave" */:		ConfigureSquareWave<T>(element, buffer, names, data); return;
	case 0x705614d5 /* "sawtoothwave" */:	ConfigureSawtoothWave<T>(element, buffer, names, data); return;

	default:								ConfigureTagVariable<T>(element, buffer, names, data); return;
	}
}

// configure an expression root (the tag hosting the expression)
template <typename T> void ConfigureExpressionRoot(const TiXmlElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[])
{
	DebugPrint("%s root %s\n", Expression::Schema<T>::NAME, element->Value());

	// width in floats (HACK)
	const int width = (sizeof(T)+sizeof(float)-1)/sizeof(float);

	// copy defaults
	float *data = static_cast<float *>(_alloca(width * sizeof(float)));
	memcpy(data, defaults, width * sizeof(float));

	// read literal values from attributes (if any)
	bool overrided = false;
	for (int i = 0; i < width; ++i)
	{
		if (element->QueryFloatAttribute(names[i], &data[i]) == TIXML_SUCCESS)
			overrided = true;
	}

	// special case: attribute variable reference
	if (element->Attribute("variable"))
	{
		ConfigureInlineVariable<T>(element, buffer, names, data);
		return;
	}

	// special case: attribute random value
	if (element->Attribute("rand"))
	{
		ConfigureRandom<T>(element, buffer, names, data);
		return;
	}

	// special case: embedded interpolator keyframes
	if (element->FirstChildElement("key"))
	{
		ConfigureInterpolator<T>(element, buffer, names, data);
		return;
	}

	// special case: no child elements
	if (!element->FirstChildElement())
	{
		// push literal data
		ConfigureLiteral<T>(element, buffer, names, data);
		return;
	}

	// special case: component elements
	for (int i = 0; i < width; ++i)
	{
		if (element->FirstChildElement(names[i]))
		{
			ConfigureConstruct<T>(element, buffer, names, data);
			return;
		}
	}

	// for each child node...
	for (const TiXmlElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
	{
		// recurse on child
		ConfigureExpression<T>(child, buffer, names, data);
	}
}

void ConfigureFloatData(const TiXmlElement *element, std::vector<unsigned int> &buffer)
{
	const char *text = element->GetText();
	size_t len = strlen(text)+1;
	char *buf = static_cast<char *>(_alloca(len));
	memcpy(buf, text, len);

	char *item = strtok(buf, " \t\n\r,;");
	while (element)
	{
		float value = float(atof(item));
		buffer.push_back(*reinterpret_cast<unsigned int *>(&value));
		item = strtok(NULL, " \t\n\r,;");
	}
}

void ConfigureVariableOperator(const TiXmlElement *element, std::vector<unsigned int> &buffer, void (*op)(DrawItemContext &), bool drawdata)
{
	unsigned int name = Hash(element->Attribute("name"));
	unsigned int type = Hash(element->Attribute("type"));
	int width;
	const char * const *names;
	const float *data;
	GetTypeData(type, width, names, data);

	Expression::Append(buffer, op);
	Expression::New<unsigned int>(buffer, name);
	Expression::New<unsigned int>(buffer, width);
	if (drawdata)
	{
		switch (width)
		{
		case 1: ConfigureExpressionRoot<float>(element, buffer, names, data); break;
		case 2: //ConfigureExpressionRoot<Vector2>(element, buffer, names, data); break;
		case 3: //ConfigureExpressionRoot<Vector3>(element, buffer, names, data); break;
		case 4: ConfigureExpressionRoot<__m128>(element, buffer, names, data); break;
		}
	}
}

void ConfigurePrimitive(const TiXmlElement *element, std::vector<unsigned int> &buffer, GLenum mode)
{
	Expression::Append(buffer, DO_glBegin, mode);
	ConfigureDrawItems(element, buffer);
	Expression::Append(buffer, DO_glEnd);
}

void ConfigureArray(const TiXmlElement *element, std::vector<unsigned int> &buffer, void (*op)(DrawItemContext &), size_t size, size_t stride)
{
	Expression::Append(buffer, op);
	Expression::New<size_t>(buffer, size);
	Expression::New<size_t>(buffer, stride);

	buffer.push_back(0);
	int start = buffer.size();
	ConfigureFloatData(element, buffer);
	buffer[start-1] = buffer.size() - start;
}

GLenum GetPrimitiveMode(const char *mode)
{
	switch (Hash(mode))
	{
	case 0xbc9567c6 /* "points" */:			return GL_POINTS; break;
	case 0xe1e4263c /* "lines" */:			return GL_LINES; break;
	case 0xc2106ab6 /* "line_loop" */:		return GL_LINE_LOOP; break;
	case 0xc6f2fa0e /* "line_strip" */:		return GL_LINE_STRIP; break;
	case 0xd8a57342 /* "triangles" */:		return GL_TRIANGLES; break;
	case 0x668b2dd8 /* "triangle_strip" */:	return GL_TRIANGLE_STRIP; break;
	case 0xcfa6904f /* "triangle_fan" */:	return GL_TRIANGLE_FAN; break;
	case 0x5667b307 /* "quads" */:			return GL_QUADS; break;
	case 0xb47cad9b /* "quad_strip" */:		return GL_QUAD_STRIP; break;
	case 0x051cb889 /* "polygon" */:		return GL_POLYGON; break;
	default:								return 0; break;
	}
}


void ConfigureDrawItem(const TiXmlElement *element, std::vector<unsigned int> &buffer)
{
	const char *label = element->Value();
	switch (Hash(label))
	{
	case 0x974c9474 /* "pushmatrix" */:
		{
			Expression::Append(buffer, DO_glPushMatrix);
			ConfigureDrawItems(element, buffer);
			Expression::Append(buffer, DO_glPopMatrix);
		}
		break;

	case 0x937cff81 /* "pushattrib" */:
		{
			GLuint mask = 0U;
			for (const TiXmlAttribute *attrib = element->FirstAttribute(); attrib != NULL; attrib = attrib->Next())
			{
				GLuint bit = 0U;
				switch (Hash(attrib->Name()))
				{
				case 0xd965bbda /* "current" */:			bit = GL_CURRENT_BIT; break;
				case 0x18ae6c91 /* "point" */:				bit = GL_POINT_BIT; break;
				case 0x17db1627 /* "line" */:				bit = GL_LINE_BIT; break;
				case 0x051cb889 /* "polygon" */:			bit = GL_POLYGON_BIT; break;
				case 0x67b14997 /* "polygon_stipple" */:	bit = GL_POLYGON_STIPPLE_BIT; break;
				case 0xccde91eb /* "pixel_mode" */:			bit = GL_PIXEL_MODE_BIT; break;
				case 0x827eb1c9 /* "lighting" */:			bit = GL_LIGHTING_BIT; break;
				case 0xa1f3723f /* "fog" */:				bit = GL_FOG_BIT; break;
				case 0x65e5b825 /* "depth_buffer" */:		bit = GL_DEPTH_BUFFER_BIT; break;
				case 0x907f6213 /* "accum_buffer" */:		bit = GL_ACCUM_BUFFER_BIT; break;
				case 0x632020be /* "stencil_buffer" */:		bit = GL_STENCIL_BUFFER_BIT; break;
				case 0xe4abbac3 /* "viewport" */:			bit = GL_VIEWPORT_BIT; break;
				case 0xe1ad931b /* "transform" */:			bit = GL_TRANSFORM_BIT; break;
				case 0xaf8bb8ce /* "enable" */:				bit = GL_ENABLE_BIT; break;
				case 0x0d759bbb /* "color_buffer" */:		bit = GL_COLOR_BUFFER_BIT; break;
				case 0x4bc809b8 /* "hint" */:				bit = GL_HINT_BIT; break;
				case 0x08d22e0f /* "eval" */:				bit = GL_EVAL_BIT; break;
				case 0x0cfb5881 /* "list" */:				bit = GL_LIST_BIT; break;
				case 0x3c6468f4 /* "texture" */:			bit = GL_TEXTURE_BIT; break;
				case 0x0adbc081 /* "scissor" */:			bit = GL_SCISSOR_BIT; break;
				case 0x13254bc4 /* "all" */:				bit = GL_ALL_ATTRIB_BITS; break;
				}
				if (attrib->IntValue())
					mask |= bit;
				else
					mask &= ~bit;
			}
			Expression::Append(buffer, DO_glPushAttrib, mask);
			ConfigureDrawItems(element, buffer);
			Expression::Append(buffer, DO_glPopAttrib);
		}
		break;

	case 0x052eb8b2 /* "pushclientattrib" */:
		{
			GLuint mask = 0U;
			for (const TiXmlAttribute *attrib = element->FirstAttribute(); attrib != NULL; attrib = attrib->Next())
			{
				GLuint bit = 0U;
				switch (Hash(attrib->Name()))
				{
				case 0x959fee19 /* "pixel_store" */:	bit = GL_CLIENT_PIXEL_STORE_BIT; break;
				case 0x20a16825 /* "vertex_array" */:	bit = GL_CLIENT_VERTEX_ARRAY_BIT; break;
				case 0x13254bc4 /* "all" */:			bit = GL_CLIENT_ALL_ATTRIB_BITS; break;
				}
				if (attrib->IntValue())
					mask |= bit;
				else
					mask &= ~bit;
			}
			Expression::Append(buffer, DO_glPushClientAttrib, mask);
			ConfigureDrawItems(element, buffer);
			Expression::Append(buffer, DO_glPopClientAttrib);
		}
		break;

	case 0xad0ecfd5 /* "translate" */:
		{
			Expression::Append(buffer, DO_glTranslatef);
			ConfigureExpressionRoot<DLTranslation>(element, buffer, sPositionNames, sPositionDefault);
		}
		break;

	case 0xa5f4fd0a /* "rotate" */:
		{
			Expression::Append(buffer, DO_glRotatef);
			ConfigureExpressionRoot<DLRotation>(element, buffer, sRotationNames, sRotationDefault);
		}
		break;

	case 0x82971c71 /* "scale" */:
		{
			Expression::Append(buffer, DO_glScalef);
			ConfigureExpressionRoot<DLScale>(element, buffer, sScaleNames, sScaleDefault);
		}
		break;

	case 0x938fc4f7 /* "loadidentity" */:
		{
			Expression::Append(buffer, DO_glLoadIdentity);
		}
		break;

	case 0x7d22a710 /* "loadmatrix" */:
		{
			Expression::Append(buffer, DO_glLoadMatrixf);
			for (int i = 0; i < 16; i++)
			{
				char name[16];
				sprintf(name, "m%d", i);
				float m = sMatrixDefault[i];
				element->QueryFloatAttribute(name, &m);
				buffer.push_back(*reinterpret_cast<unsigned int *>(&m));
			}
		}
		break;

	case 0x3807cb92 /* "multmatrix" */:
		{
			Expression::Append(buffer, DO_glMultMatrixf);
			for (int i = 0; i < 16; i++)
			{
				char name[16];
				sprintf(name, "m%d", i);
				float m = sMatrixDefault[i];
				element->QueryFloatAttribute(name, &m);
				buffer.push_back(*reinterpret_cast<unsigned int *>(&m));
			}
		}
		break;

	case 0x945367a7 /* "vertex" */:
		{
			Expression::Append(buffer, DO_glVertex3fv);
			ConfigureExpressionRoot<DLPosition>(element, buffer, sPositionNames, sPositionDefault);
		}
		break;

	case 0xe68b9c52 /* "normal" */:
		{
			Expression::Append(buffer, DO_glNormal3fv);
			ConfigureExpressionRoot<DLNormal>(element, buffer, sPositionNames, sPositionDefault);
		}
		break;

	case 0x3d7e6258 /* "color" */:
		{
			Expression::Append(buffer, DO_glColor4fv);
			ConfigureExpressionRoot<DLColor>(element, buffer, sColorNames, sColorDefault);
		}
		break;

	case 0x090aa9ab /* "index" */:
		{
			Expression::Append(buffer, DO_glIndexf);
			ConfigureExpressionRoot<DLIndex>(element, buffer, sIndexNames, sIndexDefault);
		}
		break;

	case 0xdd612dd3 /* "texcoord" */:
		{
			Expression::Append(buffer, DO_glTexCoord2fv);
			ConfigureExpressionRoot<DLTexCoord>(element, buffer, sTexCoordNames, sTexCoordDefault);
		}
		break;

	case 0x0135ab46 /* "edgeflag" */:
		{
			int flag;
			if (element->QueryIntAttribute("flag", &flag) == TIXML_SUCCESS)
			{
				Expression::Append(buffer, DO_glEdgeFlag);
				buffer.push_back(flag ? GL_TRUE : GL_FALSE);
			}
		}
		break;

	case 0x4dead571 /* "bindtexture" */:
		{
			const char *name = element->Attribute("name");
			if (name)
			{
				GLuint texture = Database::texture.Get(Hash(name));
				if (texture)
				{
					// bind the texture object
					Expression::Append(buffer, DO_glEnable);
					buffer.push_back(GL_TEXTURE_2D);
					Expression::Append(buffer, DO_glBindTexture);
					buffer.push_back(GL_TEXTURE_2D);
					buffer.push_back(texture);
				}
			}
		}
		break;

	case 0xbc9567c6 /* "points" */:
		{
			ConfigurePrimitive(element, buffer, GL_POINTS);
		}
		break;

	case 0xe1e4263c /* "lines" */:
		{
			ConfigurePrimitive(element, buffer, GL_LINES);
		}
		break;

	case 0xc2106ab6 /* "line_loop" */:
		{
			ConfigurePrimitive(element, buffer, GL_LINE_LOOP);
		}
		break;

	case 0xc6f2fa0e /* "line_strip" */:
		{
			ConfigurePrimitive(element, buffer, GL_LINE_STRIP);
		}
		break;

	case 0xd8a57342 /* "triangles" */:
		{
			ConfigurePrimitive(element, buffer, GL_TRIANGLES);
		}
		break;

	case 0x668b2dd8 /* "triangle_strip" */:
		{
			ConfigurePrimitive(element, buffer, GL_TRIANGLE_STRIP);
		}
		break;

	case 0xcfa6904f /* "triangle_fan" */:
		{
			ConfigurePrimitive(element, buffer, GL_TRIANGLE_FAN);
		}
		break;

	case 0x5667b307 /* "quads" */:
		{
			ConfigurePrimitive(element, buffer, GL_QUADS);
		}
		break;

	case 0xb47cad9b /* "quad_strip" */:
		{
			ConfigurePrimitive(element, buffer, GL_QUAD_STRIP);
		}
		break;

	case 0x051cb889 /* "polygon" */:
		{
			ConfigurePrimitive(element, buffer, GL_POLYGON);
		}
		break;

	case 0xe3e74f7e /* "blendfunc" */:
		{
			const char *src = element->Attribute("src");
			GLenum srcfactor;
			switch(Hash(src))
			{
			case 0x8b6fe763 /* "zero" */:					srcfactor = GL_ZERO; break;
			default:
			case 0xba2719ef /* "one" */:					srcfactor = GL_ONE; break;
			case 0x7fe1449a /* "src_alpha" */:				srcfactor = GL_SRC_ALPHA; break;
			case 0x1d491ada /* "one_minus_src_alpha" */:	srcfactor = GL_ONE_MINUS_SRC_ALPHA; break;
			case 0x201e0c0f /* "dst_alpha" */:				srcfactor = GL_DST_ALPHA; break;
			case 0x1ad7d24f /* "one_minus_dst_alpha" */:	srcfactor = GL_ONE_MINUS_DST_ALPHA; break;
			case 0xc913d33c /* "dst_color" */:				srcfactor = GL_DST_COLOR; break;
			case 0xc3cc067c /* "one_minus_dst_color" */:	srcfactor = GL_ONE_MINUS_DST_COLOR; break;
			case 0xe03d4f7e /* "src_alpha_saturate" */:		srcfactor = GL_SRC_ALPHA_SATURATE; break;
			}

			const char *dst = element->Attribute("dst");
			GLenum dstfactor;
			switch(Hash(dst))
			{
			default:
			case 0x8b6fe763 /* "zero" */:					dstfactor = GL_ZERO; break;
			case 0xba2719ef /* "one" */:					dstfactor = GL_ONE; break;
			case 0x4dba79b5 /* "src_color" */:				dstfactor = GL_SRC_COLOR; break;
			case 0xd1d59af5 /* "one_minus_src_color" */:	dstfactor = GL_ONE_MINUS_SRC_COLOR; break;
			case 0x7fe1449a /* "src_alpha" */:				dstfactor = GL_SRC_ALPHA; break;
			case 0x1d491ada /* "one_minus_src_alpha" */:	dstfactor = GL_ONE_MINUS_SRC_ALPHA; break;
			case 0x201e0c0f /* "dst_alpha" */:				dstfactor = GL_DST_ALPHA; break;
			case 0x1ad7d24f /* "one_minus_dst_alpha" */:	dstfactor = GL_ONE_MINUS_DST_ALPHA; break;
			}

			Expression::Append(buffer, DO_glBlendFunc);
			buffer.push_back(srcfactor);
			buffer.push_back(dstfactor);
		}
		break;

	case 0xd2cf6b75 /* "calllist" */:
		{
			const char *name = element->Attribute("name");
			if (name)
			{
				GLuint drawlist = Database::drawlist.Get(Hash(name));
				if (drawlist)
				{
					Expression::Append(buffer, DO_glCallList);
					buffer.push_back(drawlist);
				}
			}
		}
		break;

	case 0x23e2c68e /* "calldynamiclist" */:
		{
			// hacktastic!
			const char *name = element->Attribute("name");
			if (name)
			{
				const std::vector<unsigned int> &drawlist = Database::dynamicdrawlist.Get(Hash(name));
				if (drawlist.size())
				{
					for (size_t i = 0; i < drawlist.size(); ++i)
					{
						buffer.push_back(drawlist[i]);
					}
				}
			}
		}
		break;

	case 0xdf3cf9c0 /* "dynamicdrawlist" */:
		{
			const char *name = element->Attribute("name");
			if (name)
			{
				// process draw items
				unsigned int id = Hash(name);
				std::vector<unsigned int> &drawlist = Database::dynamicdrawlist.Open(id);
				ConfigureDrawItems(element, drawlist);
				Database::dynamicdrawlist.Close(id);
			}
		}
		break;

	case 0xc98b019b /* "drawlist" */:
		{
			// create a new draw list
			GLuint handle = glGenLists(1);
			glNewList(handle, GL_COMPILE);

			// register the draw list
			Database::drawlist.Put(handle, handle);

			// get (optional) parameter value
			float param = 0.0f;
			element->QueryFloatAttribute("param", &param);

			// configure the dynamic draw list
			std::vector<unsigned int> &drawlist = Database::dynamicdrawlist.Open(handle);
			ConfigureDrawItems(element, drawlist);

			// execute the dynamic draw list
			DrawItemContext context;
			context.mStream = &drawlist[0];
			context.mEnd = context.mStream + drawlist.size();
			context.mParam = param;
			context.mId = 0;
			context.mVars = &Database::variable.Open(0);
			ExecuteDrawItems(context);
			Database::variable.Close(0);

			// close the dynamic draw list
			Database::dynamicdrawlist.Close(handle);

			// finish the draw list
			glEndList();

			// use the anonymous drawlist
			Expression::Append(buffer, DO_glCallList);
			buffer.push_back(handle);
		}
		break;

	case 0x2610a4a3 /* "clientstate" */:
		{
			for (const TiXmlAttribute *attrib = element->FirstAttribute(); attrib != NULL; attrib = attrib->Next())
			{
				GLenum clientstate;
				switch (Hash(attrib->Name()))
				{
				case 0x945367a7 /* "vertex" */:		clientstate = GL_VERTEX_ARRAY; break;
				case 0xe68b9c52 /* "normal" */:		clientstate = GL_NORMAL_ARRAY; break;
				case 0x3d7e6258 /* "color" */:		clientstate = GL_COLOR_ARRAY; break;
				case 0x090aa9ab /* "index" */:		clientstate = GL_INDEX_ARRAY; break;
				case 0xdd612dd3 /* "texcoord" */:	clientstate = GL_TEXTURE_COORD_ARRAY; break;
				case 0x0135ab46 /* "edgeflag" */:	clientstate = GL_EDGE_FLAG_ARRAY; break;
				default:							clientstate = 0; break;
				}
				if (clientstate)
				{
					Expression::Append(buffer, attrib->IntValue() ? DO_glEnableClientState : DO_glDisableClientState, clientstate);
				}
			}
		}
		break;

	case 0x6298bce4 /* "vertexarray" */:
		{
			int size = 0;
			element->QueryIntAttribute("size", &size);

			int stride = 0;
			element->QueryIntAttribute("stride", &stride);

			ConfigureArray(element, buffer, DO_glVertexPointer, size, stride);
		}
		break;

	case 0x81491d33 /* "normalarray" */:
		{
			int stride = 0;
			element->QueryIntAttribute("stride", &stride);

			ConfigureArray(element, buffer, DO_glNormalPointer, 0, stride);
		}
		break;

	case 0xcce5b995 /* "colorarray" */:
		{
			int size = 0;
			element->QueryIntAttribute("size", &size);

			int stride = 0;
			element->QueryIntAttribute("stride", &stride);

			ConfigureArray(element, buffer, DO_glColorPointer, size, stride);
		}
		break;

	case 0x664ead80 /* "indexarray" */:
		{
			int stride = 0;
			element->QueryIntAttribute("stride", &stride);

			ConfigureArray(element, buffer, DO_glIndexPointer, 0, stride);
		}
		break;

	case 0x91aa3148 /* "texcoordarray" */:
		{
			int size = 0;
			element->QueryIntAttribute("size", &size);

			int stride = 0;
			element->QueryIntAttribute("stride", &stride);

			ConfigureArray(element, buffer, DO_glTexCoordPointer, size, stride);
		}
		break;

	case 0x60360ccf /* "edgeflagarray" */:
		{
			int stride = 0;
			element->QueryIntAttribute("stride", &stride);

			const char *text = element->GetText();
			size_t len = strlen(text)+1;
			char *buf = static_cast<char *>(_alloca(len));
			memcpy(buf, text, len);
			bool *data = static_cast<bool *>(_alloca(len*sizeof(bool)/2));
			int count = 0;
			char *element = strtok(buf, " \t\n\r,;");
			while (element)
			{
				data[count++] = atoi(element) != 0;
				element = strtok(NULL, " \t\n\r,;");
			}

			Expression::Append(buffer, DO_glEdgeFlagPointer);
			buffer.push_back(stride);
			buffer.push_back(count);
			for (size_t i = 0; i < (count+sizeof(unsigned int)/sizeof(bool)-1)/(sizeof(unsigned int)/sizeof(bool)); i++)
				buffer.push_back(*reinterpret_cast<unsigned int *>(&data[i*sizeof(unsigned int)/sizeof(bool)]));
		}
		break;

	case 0x0a85bb5e /* "arrayelement" */:
		{
			int index;
			if (element->QueryIntAttribute("index", &index) == TIXML_SUCCESS)
			{
				Expression::Append(buffer, DO_glArrayElement);
				buffer.push_back(index);
			}
		}
		break;

	case 0xf4de4a21 /* "drawarrays" */:
		{
			GLenum mode(GetPrimitiveMode(element->Attribute("mode")));

			int first = 0, count = 0;
			element->QueryIntAttribute("first", &first);
			element->QueryIntAttribute("count", &count);
			Expression::Append(buffer, DO_glDrawArrays);
			buffer.push_back(mode);
			buffer.push_back(first);
			buffer.push_back(count);
		}
		break;

	case 0x757eeee2 /* "drawelements" */:
		{
			GLenum mode(GetPrimitiveMode(element->Attribute("mode")));

			const char *text = element->GetText();
			size_t len = strlen(text)+1;
			char *buf = static_cast<char *>(_alloca(len));
			memcpy(buf, text, len);
			unsigned short *indices = static_cast<unsigned short *>(_alloca(len*sizeof(unsigned short)/2));
			int count = 0;
			char *element = strtok(buf, " \t\n\r,;");
			while (element)
			{
				indices[count++] = unsigned short(atoi(element));
				element = strtok(NULL, " \t\n\r,;");
			}

			Expression::Append(buffer, DO_glDrawElements);
			buffer.push_back(mode);
			buffer.push_back(count);
			for (size_t i = 0; i < (count+sizeof(unsigned int)/sizeof(unsigned short)-1)/(sizeof(unsigned int)/sizeof(unsigned short)); i++)
				buffer.push_back(*reinterpret_cast<unsigned int *>(&indices[i*sizeof(unsigned int)/sizeof(unsigned short)]));
		}
		break;

	case 0xd99ba82a /* "repeat" */:
		{
			int count = 1;
			element->QueryIntAttribute("count", &count);

			Expression::Append(buffer, DO_Repeat);
			buffer.push_back(count);

			buffer.push_back(0);
			int start = buffer.size();
			ConfigureDrawItems(element, buffer);
			buffer[start-1] = buffer.size() - start;
		}
		break;

	case 0xeb0cbd62 /* "block" */:
		{
			float start = 0.0f;
			element->QueryFloatAttribute("start", &start);
			float length = FLT_MAX;
			element->QueryFloatAttribute("length", &length);
			float scale = 1.0f;
			element->QueryFloatAttribute("scale", &scale);
			int repeat = 0;
			element->QueryIntAttribute("repeat", &repeat);

			Expression::Append(buffer, DO_Block);
			buffer.push_back(*reinterpret_cast<unsigned int *>(&start));
			buffer.push_back(*reinterpret_cast<unsigned int *>(&length));
			buffer.push_back(*reinterpret_cast<unsigned int *>(&scale));
			buffer.push_back(*reinterpret_cast<unsigned int *>(&repeat));

			buffer.push_back(0);
			int size = buffer.size();
			ConfigureDrawItems(element, buffer);
			buffer[size-1] = buffer.size() - size;
		}
		break;

	case 0xc6270703 /* "set" */:
		{
			ConfigureVariableOperator(element, buffer, DO_Set, true);
		}
		break;

	case 0x3b391274 /* "add" */:
		{
			ConfigureVariableOperator(element, buffer, DO_Add, true);
		}
		break;

	case 0xdc4e3915 /* "sub" */:
		{
			ConfigureVariableOperator(element, buffer, DO_Sub, true);
		}
		break;

	case 0xeb84ed81 /* "mul" */:
		{
			ConfigureVariableOperator(element, buffer, DO_Mul, true);
		}
		break;

	case 0xe562ab48 /* "div" */:
		{
			ConfigureVariableOperator(element, buffer, DO_Div, true);
		}
		break;

	case 0xc98f4557 /* "min" */:
		{
			ConfigureVariableOperator(element, buffer, DO_Min, true);
		}
		break;

	case 0xd7a2e319 /* "max" */:
		{
			ConfigureVariableOperator(element, buffer, DO_Max, true);
		}
		break;

	case 0x3deb1461 /* "swizzle" */:
		{
			unsigned int name = Hash(element->Attribute("name"));
			unsigned int type = Hash(element->Attribute("type"));
			int width;
			const char * const *names;
			const float *data;
			GetTypeData(type, width, names, data);

			Expression::Append(buffer, DO_Swizzle);
			buffer.push_back(name);
			buffer.push_back(width);

			for (int i = 0; i < width; ++i)
			{
				unsigned int map;
				const char *attrib = element->Attribute(names[i]);
				if (attrib == NULL)
				{
					map = ~0U;
				}
				else if (isdigit(attrib[0]))
				{
					map = attrib[0] - '0';
				}
				else if (unsigned int hash = Hash(attrib))
				{
					for (int j = 0; j < width; ++j)
					{
						if (Hash(names[j]) == hash)
						{
							map = j;
							break;
						}
					}
				}
				buffer.push_back(map);
			}
		}
		break;

	case 0x5c6e1222 /* "clear" */:
		{
			ConfigureVariableOperator(element, buffer, DO_Clear, false);
		}
		break;

#ifdef DRAWLIST_LOOP
	case 0xddef486b /* "loop" */:
		{
			unsigned int name = Hash(element->Attribute("name"));
			float from = 0.0f;
			element->QueryFloatAttribute("from", &from);
			float to = 0.0f;
			element->QueryFloatAttribute("to", &to);
			float by = from < to ? 1.0f : -1.0f;
			element->QueryFloatAttribute("by", &by);

			if ((to - from) * by <= 0)
			{
				DebugPrint("loop name=\"%s\" from=\"%f\" to=\"%f\" by=\"%f\" would never terminate\n");
				break;
			}

			Expression::Append(buffer, DO_Loop);
			buffer.push_back(name);
			buffer.push_back(*reinterpret_cast<unsigned int *>(&from));
			buffer.push_back(*reinterpret_cast<unsigned int *>(&to));
			buffer.push_back(*reinterpret_cast<unsigned int *>(&by));

			buffer.push_back(0);
			int start = buffer.size();
			ConfigureDrawItems(element, buffer);
			buffer[start-1] = buffer.size() - start;
		}
		break;
#endif

#ifdef DRAWLIST_EMITTER
	case 0x576b09cd /* "emitter" */:
		{
			int count = 1;
			element->QueryIntAttribute("count", &count);
			float period = 1.0f;
			element->QueryFloatAttribute("period", &period);
			float x = 0.0f, y = 0.0f, a = 0.0f;
			element->QueryFloatAttribute("x", &x);
			element->QueryFloatAttribute("y", &y);
			element->QueryFloatAttribute("angle", &a);

			Expression::Append(buffer, DO_Emitter);
			buffer.push_back(count);
			buffer.push_back(*reinterpret_cast<unsigned int *>(&period));
			buffer.push_back(*reinterpret_cast<unsigned int *>(&x));
			buffer.push_back(*reinterpret_cast<unsigned int *>(&y));
			buffer.push_back(*reinterpret_cast<unsigned int *>(&a));

			buffer.push_back(0);
			int start = buffer.size();
			ConfigureDrawItems(element, buffer);
			buffer[start-1] = buffer.size() - start;
		}
		break;
#endif

	default:
		break;
	}
}

void ConfigureDrawItems(const TiXmlElement *element, std::vector<unsigned int> &buffer)
{
	// process child elements
	for (const TiXmlElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
	{
		ConfigureDrawItem(child, buffer);
	}
}

#ifdef DRAWLIST_EMITTER
float Determinant4f(const float m[16])
{
	return
		m[12]*m[9]*m[6]*m[3]-
		m[8]*m[13]*m[6]*m[3]-
		m[12]*m[5]*m[10]*m[3]+
		m[4]*m[13]*m[10]*m[3]+
		m[8]*m[5]*m[14]*m[3]-
		m[4]*m[9]*m[14]*m[3]-
		m[12]*m[9]*m[2]*m[7]+
		m[8]*m[13]*m[2]*m[7]+
		m[12]*m[1]*m[10]*m[7]-
		m[0]*m[13]*m[10]*m[7]-
		m[8]*m[1]*m[14]*m[7]+
		m[0]*m[9]*m[14]*m[7]+
		m[12]*m[5]*m[2]*m[11]-
		m[4]*m[13]*m[2]*m[11]-
		m[12]*m[1]*m[6]*m[11]+
		m[0]*m[13]*m[6]*m[11]+
		m[4]*m[1]*m[14]*m[11]-
		m[0]*m[5]*m[14]*m[11]-
		m[8]*m[5]*m[2]*m[15]+
		m[4]*m[9]*m[2]*m[15]+
		m[8]*m[1]*m[6]*m[15]-
		m[0]*m[9]*m[6]*m[15]-
		m[4]*m[1]*m[10]*m[15]+
		m[0]*m[5]*m[10]*m[15];
}

bool InvertMatrix4f(float i[16], const float m[16])
{
	float x=Determinant4f(m);
	if (x==0) return false;
	x = 1.0f / x;
	i[ 0]= (-m[13] * m[10] * m[ 7] + m[ 9] * m[14] * m[ 7] + m[13] * m[ 6] * m[11] - m[ 5] * m[14] * m[11] - m[ 9] * m[ 6] * m[15] + m[ 5] * m[10] * m[15]) * x;
	i[ 4]= ( m[12] * m[10] * m[ 7] - m[ 8] * m[14] * m[ 7] - m[12] * m[ 6] * m[11] + m[ 4] * m[14] * m[11] + m[ 8] * m[ 6] * m[15] - m[ 4] * m[10] * m[15]) * x;
	i[ 8]= (-m[12] * m[ 9] * m[ 7] + m[ 8] * m[13] * m[ 7] + m[12] * m[ 5] * m[11] - m[ 4] * m[13] * m[11] - m[ 8] * m[ 5] * m[15] + m[ 4] * m[ 9] * m[15]) * x;
	i[12]= ( m[12] * m[ 9] * m[ 6] - m[ 8] * m[13] * m[ 6] - m[12] * m[ 5] * m[10] + m[ 4] * m[13] * m[10] + m[ 8] * m[ 5] * m[14] - m[ 4] * m[ 9] * m[14]) * x;
	i[ 1]= ( m[13] * m[10] * m[ 3] - m[ 9] * m[14] * m[ 3] - m[13] * m[ 2] * m[11] + m[ 1] * m[14] * m[11] + m[ 9] * m[ 2] * m[15] - m[ 1] * m[10] * m[15]) * x;
	i[ 5]= (-m[12] * m[10] * m[ 3] + m[ 8] * m[14] * m[ 3] + m[12] * m[ 2] * m[11] - m[ 0] * m[14] * m[11] - m[ 8] * m[ 2] * m[15] + m[ 0] * m[10] * m[15]) * x;
	i[ 9]= ( m[12] * m[ 9] * m[ 3] - m[ 8] * m[13] * m[ 3] - m[12] * m[ 1] * m[11] + m[ 0] * m[13] * m[11] + m[ 8] * m[ 1] * m[15] - m[ 0] * m[ 9] * m[15]) * x;
	i[13]= (-m[12] * m[ 9] * m[ 2] + m[ 8] * m[13] * m[ 2] + m[12] * m[ 1] * m[10] - m[ 0] * m[13] * m[10] - m[ 8] * m[ 1] * m[14] + m[ 0] * m[ 9] * m[14]) * x;
	i[ 2]= (-m[13] * m[ 6] * m[ 3] + m[ 5] * m[14] * m[ 3] + m[13] * m[ 2] * m[ 7] - m[ 1] * m[14] * m[ 7] - m[ 5] * m[ 2] * m[15] + m[ 1] * m[ 6] * m[15]) * x;
	i[ 6]= ( m[12] * m[ 6] * m[ 3] - m[ 4] * m[14] * m[ 3] - m[12] * m[ 2] * m[ 7] + m[ 0] * m[14] * m[ 7] + m[ 4] * m[ 2] * m[15] - m[ 0] * m[ 6] * m[15]) * x;
	i[10]= (-m[12] * m[ 5] * m[ 3] + m[ 4] * m[13] * m[ 3] + m[12] * m[ 1] * m[ 7] - m[ 0] * m[13] * m[ 7] - m[ 4] * m[ 1] * m[15] + m[ 0] * m[ 5] * m[15]) * x;
	i[14]= ( m[12] * m[ 5] * m[ 2] - m[ 4] * m[13] * m[ 2] - m[12] * m[ 1] * m[ 6] + m[ 0] * m[13] * m[ 6] + m[ 4] * m[ 1] * m[14] - m[ 0] * m[ 5] * m[14]) * x;
	i[ 3]= ( m[ 9] * m[ 6] * m[ 3] - m[ 5] * m[10] * m[ 3] - m[ 9] * m[ 2] * m[ 7] + m[ 1] * m[10] * m[ 7] + m[ 5] * m[ 2] * m[11] - m[ 1] * m[ 6] * m[11]) * x;
	i[ 7]= (-m[ 8] * m[ 6] * m[ 3] + m[ 4] * m[10] * m[ 3] + m[ 8] * m[ 2] * m[ 7] - m[ 0] * m[10] * m[ 7] - m[ 4] * m[ 2] * m[11] + m[ 0] * m[ 6] * m[11]) * x;
	i[11]= ( m[ 8] * m[ 5] * m[ 3] - m[ 4] * m[ 9] * m[ 3] - m[ 8] * m[ 1] * m[ 7] + m[ 0] * m[ 9] * m[ 7] + m[ 4] * m[ 1] * m[11] - m[ 0] * m[ 5] * m[11]) * x;
	i[15]= (-m[ 8] * m[ 5] * m[ 2] + m[ 4] * m[ 9] * m[ 2] + m[ 8] * m[ 1] * m[ 6] - m[ 0] * m[ 9] * m[ 6] - m[ 4] * m[ 1] * m[10] + m[ 0] * m[ 5] * m[10]) * x;
	return true;
}

void MultiplyMatrix4f(float m[16], float a[16], float b[16])
{
	m[ 0] = a[ 0] * b[ 0] + a[ 1] * b[ 4] + a[ 2] * b[ 8] + a[ 3] * b[12];
	m[ 1] = a[ 0] * b[ 1] + a[ 1] * b[ 5] + a[ 2] * b[ 9] + a[ 3] * b[13];
	m[ 2] = a[ 0] * b[ 2] + a[ 1] * b[ 6] + a[ 2] * b[10] + a[ 3] * b[14];
	m[ 3] = a[ 0] * b[ 3] + a[ 1] * b[ 7] + a[ 2] * b[11] + a[ 3] * b[15];
	m[ 4] = a[ 4] * b[ 0] + a[ 5] * b[ 4] + a[ 6] * b[ 8] + a[ 7] * b[12];
	m[ 5] = a[ 4] * b[ 1] + a[ 5] * b[ 5] + a[ 6] * b[ 9] + a[ 7] * b[13];
	m[ 6] = a[ 4] * b[ 2] + a[ 5] * b[ 6] + a[ 6] * b[10] + a[ 7] * b[14];
	m[ 7] = a[ 4] * b[ 3] + a[ 5] * b[ 7] + a[ 6] * b[11] + a[ 7] * b[15];
	m[ 8] = a[ 8] * b[ 0] + a[ 9] * b[ 4] + a[10] * b[ 8] + a[11] * b[12];
	m[ 9] = a[ 8] * b[ 1] + a[ 9] * b[ 5] + a[10] * b[ 9] + a[11] * b[13];
	m[10] = a[ 8] * b[ 2] + a[ 9] * b[ 6] + a[10] * b[10] + a[11] * b[14];
	m[11] = a[ 8] * b[ 3] + a[ 9] * b[ 7] + a[10] * b[11] + a[11] * b[15];
	m[12] = a[12] * b[ 0] + a[13] * b[ 4] + a[14] * b[ 8] + a[15] * b[12];
	m[13] = a[12] * b[ 1] + a[13] * b[ 5] + a[14] * b[ 9] + a[15] * b[13];
	m[14] = a[12] * b[ 2] + a[13] * b[ 6] + a[14] * b[10] + a[15] * b[14];
	m[15] = a[12] * b[ 3] + a[13] * b[ 7] + a[14] * b[11] + a[15] * b[15];
}
#endif

#pragma optimize( "t", on )
void ExecuteDrawItems(DrawItemContext &aContext)
{
	while (aContext.mStream < aContext.mEnd)
		Expression::Evaluate<void>(aContext);
}
#pragma optimize( "", on )

void RebuildDrawlists(void)
{
	// for each entry in the drawlist database...
	for (Database::Typed<GLuint>::Iterator itor(&Database::drawlist); itor.IsValid(); ++itor)
	{
		// recreate the draw list
		GLuint handle = itor.GetValue();
		glNewList(handle, GL_COMPILE);

		// TO DO: recover parameter value
		float param = 0.0f;

		// process draw items
		// TO DO: recover id value
		const std::vector<unsigned int> &drawlist = Database::dynamicdrawlist.Get(handle);
		if (drawlist.size() > 0)
		{
			DrawItemContext context;
			context.mStream = &drawlist[0];
			context.mEnd = context.mStream + drawlist.size();
			context.mParam = param;
			context.mId = 0;
			context.mVars = &Database::variable.Open(context.mId);
			ExecuteDrawItems(context);
			Database::variable.Close(context.mId);
		}

		// finish the draw list
		glEndList();
	}
}

void RenderDrawlist(unsigned int aId, float aTime, const Transform2 &aTransform)
{
	// skip if not visible
	if (aTime < 0)
		return;

	// get the dynamic draw list
	const std::vector<unsigned int> &buffer = Database::dynamicdrawlist.Get(aId);

	// skip if empty
	if (buffer.empty())
		return;

	// push a transform
	glPushMatrix();

	// load matrix
	glTranslatef(aTransform.p.x, aTransform.p.y, 0);
	glRotatef(aTransform.a*180/float(M_PI), 0.0f, 0.0f, 1.0f);

	// execute the deferred draw list
	DrawItemContext context;
	context.mStream = &buffer[0];
	context.mEnd = context.mStream + buffer.size();
	context.mParam = aTime;
	context.mId = aId;
	context.mVars = &Database::variable.Open(aId);
	ExecuteDrawItems(context);
	Database::variable.Close(aId);

	// reset the transform
	glPopMatrix();
};

#include "StdAfx.h"
#include "Drawlist.h"
#include "Variable.h"
#include "Texture.h"
#include "Interpolator.h"
#include "Noise.h"

#include "Expression.h"
#include "ExpressionConfigure.h"


//
// DEFINES
//

// enable "loop" drawlist element
#define DRAWLIST_LOOP


//
// FORWARD DECLARATIONS
//

// execute a dynamic draw list
static void ExecuteDrawItems(EntityContext &aContext);


//
// DATA TYPES
//

typedef float DLScalar;
const char * const sScalarNames[] = { "value" };
const float sScalarDefault[] = { 0.0f };
const int sScalarWidth = 1;

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

static const char * const sMatrixNames[] = { "m0", "m1", "m2", "m3", "m4", "m5", "m6", "m7", "m8", "m9", "m10", "m11", "m12", "m13", "m14", "m15" };
static const float sMatrixDefault[] = { 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f };
static const int sMatrixWidth = 16;

void GetTypeData(unsigned int type, int &width, const char * const *&names, const float *&data)
{
	switch (type)
	{
	default:
	case 0xaeebcbdd /* "scalar" */:		names = sScalarNames; data = sScalarDefault; width = sScalarWidth; break;
	case 0x934f4e0a /* "position" */:	names = sPositionNames; data = sPositionDefault; width = sPositionWidth; break;
	case 0xe68b9c52 /* "normal" */:		names = sNormalNames; data = sNormalDefault; width = sNormalWidth; break;
	case 0xad0ecfd5 /* "translate" */:	names = sTranslationNames, data = sTranslationDefault; width = sTranslationWidth; break;
	case 0x21ac415f /* "rotation" */:	names = sRotationNames; data = sRotationDefault; width = sRotationWidth; break;
	case 0x82971c71 /* "scale" */:		names = sScaleNames; data = sScaleDefault; width = sScaleWidth; break;
	case 0x3d7e6258 /* "color" */:		names = sColorNames; data = sColorDefault; width = sColorWidth; break;
	case 0xdd612dd3 /* "texcoord" */:	names = sTexCoordNames; data = sTexCoordDefault; width = sTexCoordWidth; break;
	case 0x15c2f8ec /* "matrix" */:		names = sMatrixNames; data = sMatrixDefault; width = sMatrixWidth; break;
	}
}

namespace Database
{
	Typed<std::vector<unsigned int> > dynamicdrawlist(0xdf3cf9c0 /* "dynamicdrawlist" */);
	Typed<GLuint> drawlist(0xc98b019b /* "drawlist" */);

	namespace Loader
	{
		static void DynamicDrawlistConfigure(unsigned int aId, const tinyxml2::XMLElement *element)
		{
			std::vector<unsigned int> &buffer = Database::dynamicdrawlist.Open(aId);
			assert(buffer.size() == 0);
			ConfigureDrawItems(aId, element, buffer);
			Database::dynamicdrawlist.Close(aId);
		}
		Configure dynamicdrawlistconfigure(0xdf3cf9c0 /* "dynamicdrawlist" */, DynamicDrawlistConfigure);

		static void DrawlistConfigure(unsigned int aId, const tinyxml2::XMLElement *element)
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
			ConfigureDrawItems(aId, element, drawlist);

			// execute the dynamic draw list
			EntityContext context(&drawlist[0], drawlist.size(), param, aId);
			ExecuteDrawItems(context);

			// close the dynamic draw list
			Database::dynamicdrawlist.Close(handle);

			// finish the draw list
			glEndList();
		}
		Configure drawlistconfigure(0xc98b019b /* "drawlist" */, DrawlistConfigure);

		static void VariableConfigure(unsigned int aId, const tinyxml2::XMLElement *element)
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
		Configure variableconfigure(0x19385305 /* "variable" */, VariableConfigure);
	}

}


//
// DRAWLIST VARIABLE OPERATOR
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
bool EvaluateVariableOperator(EntityContext &aContext)
{
	const VariableOperator op = Expression::Read<VariableOperator>(aContext);
	const unsigned int name = Expression::Read<unsigned int>(aContext);
	const int width = Expression::Read<int>(aContext);
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
		op(v, value.m128_f32[i]);
		aContext.mVars->Close(name+i);
	}
//	Database::variable.Close(aContext.mId);
	return true;
}



//
// DRAWLIST OPERATIONS
//

void DO_Begin(EntityContext &aContext)
{
	glBegin(Expression::Read<GLenum>(aContext));
}

void DO_BindTexture(EntityContext &aContext)
{
	const GLenum target(Expression::Read<GLenum>(aContext));
	const GLuint texture(Expression::Read<GLuint>(aContext));
	glBindTexture(target, texture);
}

void DO_BlendFunc(EntityContext &aContext)
{
	const GLenum sfactor(Expression::Read<GLenum>(aContext));
	const GLenum dfactor(Expression::Read<GLenum>(aContext));
	glBlendFunc(sfactor, dfactor);
}

void DO_CallList(EntityContext &aContext)
{
	glCallList(Expression::Read<GLuint>(aContext));
}

void DO_Color(EntityContext &aContext)
{
	const DLColor value(Expression::Evaluate<DLColor>(aContext));
	glColor4fv(value.m128_f32);
}

void DO_Disable(EntityContext &aContext)
{
	glDisable(Expression::Read<GLenum>(aContext));
}

void DO_Enable(EntityContext &aContext)
{
	glEnable(Expression::Read<GLenum>(aContext));
}

void DO_End(EntityContext &aContext)
{
	glEnd();
}

void DO_LineWidth(EntityContext &aContext)
{
	const GLfloat width(Expression::Read<GLfloat>(aContext));
	glLineWidth(width);
}

void DO_LineWidthWorld(EntityContext &aContext)
{
	const GLfloat width(Expression::Read<GLfloat>(aContext) * float(SCREEN_HEIGHT) / VIEW_SIZE);
	glLineWidth(width);
}

void DO_LoadIdentity(EntityContext &aContext)
{
	glLoadIdentity();
}

void DO_LoadMatrix(EntityContext &aContext)
{
	glLoadMatrixf(reinterpret_cast<const GLfloat *>(aContext.mStream));
	aContext.mStream += (16*sizeof(GLfloat)+sizeof(unsigned int)-1)/sizeof(unsigned int);
}

void DO_MultMatrix(EntityContext &aContext)
{
	glMultMatrixf(reinterpret_cast<const GLfloat *>(aContext.mStream));
	aContext.mStream += (16*sizeof(GLfloat)+sizeof(unsigned int)-1)/sizeof(unsigned int);
}

void DO_Normal(EntityContext &aContext)
{
	const DLNormal value(Expression::Evaluate<DLNormal>(aContext));
	glNormal3fv(value.m128_f32);
}

void DO_PointSize(EntityContext &aContext)
{
	const GLfloat size(Expression::Read<GLfloat>(aContext));
	glPointSize(size);
}

void DO_PointSizeWorld(EntityContext &aContext)
{
	const GLfloat size(Expression::Read<GLfloat>(aContext) * float(SCREEN_HEIGHT) / VIEW_SIZE);
	glPointSize(size);
}

void DO_PopAttrib(EntityContext &aContext)
{
	glPopAttrib();
}

void DO_PopMatrix(EntityContext &aContext)
{
	glPopMatrix();
}

void DO_PushAttrib(EntityContext &aContext)
{
	glPushAttrib(Expression::Read<GLbitfield>(aContext));
}

void DO_PushMatrix(EntityContext &aContext)
{
	glPushMatrix();
}

void DO_Rotate(EntityContext &aContext)
{
	const float value(Expression::Evaluate<DLRotation>(aContext));
	glRotatef(value, 0, 0, 1);
}

void DO_Scale(EntityContext &aContext)
{
	const DLScale value(Expression::Evaluate<DLScale>(aContext));
	glScalef(value.m128_f32[0], value.m128_f32[1], value.m128_f32[2]);
}

void DO_TexCoord(EntityContext &aContext)
{
	const DLTexCoord value(Expression::Evaluate<DLTexCoord>(aContext));
	glTexCoord2fv(value.m128_f32);
}

void DO_TexEnvi(EntityContext &aContext)
{
	const GLenum pname(Expression::Read<GLint>(aContext));
	const GLint param(Expression::Read<GLint>(aContext));
	glTexEnvi( GL_TEXTURE_ENV, pname, param );
}

void DO_Translate(EntityContext &aContext)
{
	const DLTranslation value(Expression::Evaluate<DLTranslation>(aContext));
	glTranslatef(value.m128_f32[0], value.m128_f32[1], value.m128_f32[2]);
}

void DO_Vertex(EntityContext &aContext)
{
	const DLPosition value(Expression::Evaluate<DLPosition>(aContext));
	glVertex3fv(value.m128_f32);
}

void DO_Repeat(EntityContext &aContext)
{
	const int repeat(Expression::Read<int>(aContext));
	const unsigned int size(Expression::Read<unsigned int>(aContext));
	EntityContext context(aContext.mStream, size, aContext.mParam, aContext.mId, aContext.mVars);
	for (int i = 0; i < repeat; i++)
	{
		ExecuteDrawItems(context);
		context.Restart();
	}
	aContext.mStream += size;
}

void DO_Block(EntityContext &aContext)
{
	const float start(Expression::Read<float>(aContext));
	const float length(Expression::Read<float>(aContext));
	const float scale(Expression::Read<float>(aContext));
	const int repeat(Expression::Read<int>(aContext));
	const unsigned int size(Expression::Read<unsigned int>(aContext));
	float t = aContext.mParam - start;
	if (t >= 0.0f && length > 0.0f)
	{
		int loop = FloorToInt(t / length);
		if (repeat < 0 || loop <= repeat)
		{
			t -= loop * length;
			t *= scale;
			EntityContext context(aContext.mStream, size, t, aContext.mId, aContext.mVars);
			ExecuteDrawItems(context);
		}
	}
	aContext.mStream += size;
}

void DO_Swizzle(EntityContext &aContext)
{
	const unsigned int name(Expression::Read<unsigned int>(aContext));
	const int width(Expression::Read<int>(aContext));
//	Database::Typed<float> &variables = Database::variable.Open(aContext.mId);
	float *temp = static_cast<float *>(_alloca(width * sizeof(float)));
	for (int i = 0; i < width; i++)
		temp[i] = aContext.mVars->Get(name + Expression::Read<unsigned int>(aContext));
	for (int i = 0; i < width; i++)
		aContext.mVars->Put(name + i, temp[i]);
//	Database::variable.Close(aContext.mId);
}

void DO_Clear(EntityContext &aContext)
{
	const unsigned int name(Expression::Read<unsigned int>(aContext));
	const int width(Expression::Read<int>(aContext));
//	Database::Typed<float> &variables = Database::variable.Open(aContext.mId);
	for (int i = 0; i < width; i++)
		aContext.mVars->Delete(name+i);
//	Database::variable.Close(aContext.mId);
}

#ifdef DRAWLIST_LOOP
void DO_Loop(EntityContext &aContext)
{
	const unsigned int name = Expression::Read<unsigned int>(aContext);
	const float from = Expression::Read<float>(aContext);
	const float to   = Expression::Read<float>(aContext);
	const float by   = Expression::Read<float>(aContext);
	const unsigned int size = Expression::Read<unsigned int>(aContext);

//		Database::Typed<float> &variables = Database::variable.Open(aContext.mId);
	EntityContext context(aContext.mStream, size, aContext.mParam, aContext.mId, aContext.mVars);
	if (by > 0)
	{
		for (float value = from; value <= to; value += by)
		{
			context.mVars->Put(name, value);
			ExecuteDrawItems(context);
			context.Restart();
		}
	}
	else
	{
		for (float value = from; value >= to; value += by)
		{
			context.mVars->Put(name, value);
			ExecuteDrawItems(context);
			context.Restart();
		}
	}
	context.mVars->Delete(name);
//		Database::variable.Close(aContext.mId);

	aContext.mStream += size;
}
#endif

void ConfigureVariableOperator(const tinyxml2::XMLElement *element, std::vector<unsigned int> &buffer, VariableOperator op)
{
	const unsigned int name = Hash(element->Attribute("name"));
	const unsigned int type = Hash(element->Attribute("type"));
	int width;
	const char * const *names;
	const float *data;
	GetTypeData(type, width, names, data);

	Expression::Append(buffer, EvaluateVariableOperator, op, name, width);
	switch (width)
	{
	case 1: Expression::Loader<float>::ConfigureRoot(element, buffer, names, data); break;
	case 2: //Expression::Loader<Vector2>::ConfigureRoot(element, buffer, names, data); break;
	case 3: //Expression::Loader<Vector3>::ConfigureRoot(element, buffer, names, data); break;
	case 4: Expression::Loader<__m128>::ConfigureRoot(element, buffer, names, data); break;
	}
}

void ConfigureVariableClear(const tinyxml2::XMLElement *element, std::vector<unsigned int> &buffer)
{
	const unsigned int name = Hash(element->Attribute("name"));
	const unsigned int type = Hash(element->Attribute("type"));
	int width;
	const char * const *names;
	const float *data;
	GetTypeData(type, width, names, data);

	Expression::Append(buffer, DO_Clear, name, width);
}


void ConfigurePrimitive(GLenum mode, unsigned int aId, const tinyxml2::XMLElement *element, std::vector<unsigned int> &buffer)
{
	Expression::Append(buffer, DO_Begin, mode);
	ConfigureDrawItems(aId, element, buffer);
	Expression::Append(buffer, DO_End);
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

static void ConfigureMatrix(const tinyxml2::XMLElement *element, float m[])
{
	for (int i = 0; i < 16; i++)
	{
		char name[16];
		sprintf(name, "m%d", i);
		m[i] = sMatrixDefault[i];
		element->QueryFloatAttribute(name, &m[i]);
	}
}


void ConfigureDrawItem(unsigned int aId, const tinyxml2::XMLElement *element, std::vector<unsigned int> &buffer)
{
	const char *label = element->Value();
	switch (Hash(label))
	{
		//
		// TRANSFORM COMMANDS

	case 0x974c9474 /* "pushmatrix" */:
		{
			Expression::Append(buffer, DO_PushMatrix);
			ConfigureDrawItems(aId, element, buffer);
			Expression::Append(buffer, DO_PopMatrix);
		}
		break;

	case 0xad0ecfd5 /* "translate" */:
		{
			Expression::Append(buffer, DO_Translate);
			Expression::Loader<DLTranslation>::ConfigureRoot(element, buffer, sPositionNames, sPositionDefault);
		}
		break;

	case 0xa5f4fd0a /* "rotate" */:
		{
			Expression::Append(buffer, DO_Rotate);
			Expression::Loader<DLRotation>::ConfigureRoot(element, buffer, sRotationNames, sRotationDefault);
		}
		break;

	case 0x82971c71 /* "scale" */:
		{
			Expression::Append(buffer, DO_Scale);
			Expression::Loader<DLScale>::ConfigureRoot(element, buffer, sScaleNames, sScaleDefault);
		}
		break;

	case 0x938fc4f7 /* "loadidentity" */:
		{
			Expression::Append(buffer, DO_LoadIdentity);
		}
		break;

	case 0x7d22a710 /* "loadmatrix" */:
		{
			float m[16];
			ConfigureMatrix(element, m);
			Expression::Append(buffer, DO_LoadMatrix);
			Expression::Append(buffer, m);
		}
		break;

	case 0x3807cb92 /* "multmatrix" */:
		{
			float m[16];
			ConfigureMatrix(element, m);
			Expression::Append(buffer, DO_MultMatrix);
			Expression::Append(buffer, m);
		}
		break;

		//
		// GEOMETRY COMMANDS

	case 0x945367a7 /* "vertex" */:
		{
			Expression::Append(buffer, DO_Vertex);
			Expression::Loader<DLPosition>::ConfigureRoot(element, buffer, sPositionNames, sPositionDefault);
		}
		break;

	case 0xe68b9c52 /* "normal" */:
		{
			Expression::Append(buffer, DO_Normal);
			Expression::Loader<DLNormal>::ConfigureRoot(element, buffer, sNormalNames, sNormalDefault);
		}
		break;

	case 0x3d7e6258 /* "color" */:
		{
			Expression::Append(buffer, DO_Color);
			Expression::Loader<DLColor>::ConfigureRoot(element, buffer, sColorNames, sColorDefault);
		}
		break;

	case 0xdd612dd3 /* "texcoord" */:
		{
			Expression::Append(buffer, DO_TexCoord);
			Expression::Loader<DLTexCoord>::ConfigureRoot(element, buffer, sTexCoordNames, sTexCoordDefault);
		}
		break;

		//
		// PRIMITIVE COMMANDS

	case 0xbc9567c6 /* "points" */:
		{
			float size = 0.0f;
			element->QueryFloatAttribute("size", &size);
			if (size != 0.0f)
			{
				Expression::Append(buffer, DO_PushAttrib, GL_POINT_BIT);
				Expression::Append(buffer, DO_PointSizeWorld, size);
			}
			ConfigurePrimitive(GL_POINTS, aId, element, buffer);
			if (size != 0.0f)
			{
				Expression::Append(buffer, DO_PopAttrib);
			}
		}
		break;

	case 0xe1e4263c /* "lines" */:
		{
			float width = 0.0f;
			element->QueryFloatAttribute("width", &width);
			if (width != 0.0f)
			{
				Expression::Append(buffer, DO_PushAttrib, GL_LINE_BIT);
				Expression::Append(buffer, DO_LineWidthWorld, width);
			}
			ConfigurePrimitive(GL_LINES, aId, element, buffer);
			if (width != 0.0f)
			{
				Expression::Append(buffer, DO_PopAttrib);
			}
		}
		break;

	case 0xc2106ab6 /* "line_loop" */:
		{
			float width = 0.0f;
			element->QueryFloatAttribute("width", &width);
			if (width != 0.0f)
			{
				Expression::Append(buffer, DO_PushAttrib, GL_LINE_BIT);
				Expression::Append(buffer, DO_LineWidthWorld, width);
			}
			ConfigurePrimitive(GL_LINE_LOOP, aId, element, buffer);
			if (width != 0.0f)
			{
				Expression::Append(buffer, DO_PopAttrib);
			}
		}
		break;

	case 0xc6f2fa0e /* "line_strip" */:
		{
			float width = 0.0f;
			element->QueryFloatAttribute("width", &width);
			if (width != 0.0f)
			{
				Expression::Append(buffer, DO_PushAttrib, GL_LINE_BIT);
				Expression::Append(buffer, DO_LineWidthWorld, width);
			}
			ConfigurePrimitive(GL_LINE_STRIP, aId, element, buffer);
			if (width != 0.0f)
			{
				Expression::Append(buffer, DO_PopAttrib);
			}
		}
		break;

	case 0xd8a57342 /* "triangles" */:
		{
			ConfigurePrimitive(GL_TRIANGLES, aId, element, buffer);
		}
		break;

	case 0x668b2dd8 /* "triangle_strip" */:
		{
			ConfigurePrimitive(GL_TRIANGLE_STRIP, aId, element, buffer);
		}
		break;

	case 0xcfa6904f /* "triangle_fan" */:
		{
			ConfigurePrimitive(GL_TRIANGLE_FAN, aId, element, buffer);
		}
		break;

	case 0x5667b307 /* "quads" */:
		{
			ConfigurePrimitive(GL_QUADS, aId, element, buffer);
		}
		break;

	case 0xb47cad9b /* "quad_strip" */:
		{
			ConfigurePrimitive(GL_QUAD_STRIP, aId, element, buffer);
		}
		break;

	case 0x051cb889 /* "polygon" */:
		{
			ConfigurePrimitive(GL_POLYGON, aId, element, buffer);
		}
		break;

		//
		// RENDERSTATE COMMANDS

	case 0x937cff81 /* "pushattrib" */:
		{
			GLuint mask = 0U;
			for (const tinyxml2::XMLAttribute *attrib = element->FirstAttribute(); attrib != NULL; attrib = attrib->Next())
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
				if (attrib->BoolValue())
					mask |= bit;
				else
					mask &= ~bit;
			}
			Expression::Append(buffer, DO_PushAttrib, mask);
			ConfigureDrawItems(aId, element, buffer);
			Expression::Append(buffer, DO_PopAttrib);
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
					Expression::Append(buffer, DO_Enable, GL_TEXTURE_2D);
					Expression::Append(buffer, DO_BindTexture, GL_TEXTURE_2D, texture);
				}
				else
				{
					DebugPrint("Missing texture %s", name);
				}
			}
		}

	case 0x059e3a91 /* "texenv" */:
		{
			// set blend mode
			GLenum blendmode;
			switch (Hash(element->Attribute("mode")))
			{
			default:
			case 0x818f75ae /* "modulate" */:	blendmode = GL_MODULATE; break;
			case 0xde15f6ae /* "decal" */:		blendmode = GL_DECAL; break;
			case 0x0bbc40d8 /* "blend" */:		blendmode = GL_BLEND; break;
			case 0xa13884c3 /* "replace" */:	blendmode = GL_REPLACE; break;
			}
			Expression::Append(buffer, DO_TexEnvi, GL_TEXTURE_ENV_MODE, blendmode);
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

			Expression::Append(buffer, DO_BlendFunc, srcfactor, dstfactor);
		}
		break;

		//
		// DRAWLIST COMMANDS

	case 0xc98b019b /* "drawlist" */:
		{
			// create a new draw list
			GLuint handle = glGenLists(1);
			glNewList(handle, GL_COMPILE);

			// register the draw list
			Database::drawlist.Put(handle, handle);

			// configure the dynamic draw list
			std::vector<unsigned int> &drawlist = Database::dynamicdrawlist.Open(handle);
			ConfigureDrawItems(aId, element, drawlist);

			// execute the dynamic draw list
			EntityContext context(&drawlist[0], drawlist.size(), 0.0f, aId);
			ExecuteDrawItems(context);

			// close the dynamic draw list
			Database::dynamicdrawlist.Close(handle);

			// finish the draw list
			glEndList();

			// use the anonymous drawlist
			Expression::Append(buffer, DO_CallList, handle);
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
					Expression::Append(buffer, DO_CallList, drawlist);
				}
				else
				{
					DebugPrint("Missing drawlist %s\n", name);
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
				ConfigureDrawItems(id, element, drawlist);
				Database::dynamicdrawlist.Close(id);
			}
		}
		break;

	case 0x23e2c68e /* "calldynamiclist" */:
		{
			// hacktastic!
			const char *name = element->Attribute("name");
			if (name)
			{
				// TO DO: call drawlist at runtime instead of "inlining" it
				const std::vector<unsigned int> &drawlist = Database::dynamicdrawlist.Get(Hash(name));
				if (drawlist.size())
				{
					buffer.insert(buffer.end(), drawlist.begin(), drawlist.end());
				}
				else
				{
					DebugPrint("Missing dynamic drawlist %s\n", name);
				}
			}
		}
		break;

		//
		// GROUPING COMMANDS

	case 0xd99ba82a /* "repeat" */:
		{
			int count = 1;
			element->QueryIntAttribute("count", &count);

			Expression::Append(buffer, DO_Repeat, count);

			size_t buffer_size_at = buffer.size();
			Expression::Alloc(buffer, sizeof(unsigned int));
			size_t start = buffer.size();
			ConfigureDrawItems(aId, element, buffer);
			*new (buffer.data() + buffer_size_at) unsigned int = unsigned int(buffer.size() - start);
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

			Expression::Append(buffer, DO_Block, start, length, scale, repeat);

			size_t buffer_size_at = buffer.size();
			Expression::Alloc(buffer, sizeof(unsigned int));
			size_t size = buffer.size();
			ConfigureDrawItems(aId, element, buffer);
			*new (buffer.data() + buffer_size_at) unsigned int = unsigned int(buffer.size() - size);
		}
		break;

#ifdef DRAWLIST_LOOP
	case 0xddef486b /* "loop" */:
		{
			const unsigned int name = Hash(element->Attribute("name"));
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

			Expression::Append(buffer, DO_Loop, name, from, to, by);

			size_t buffer_size_at = buffer.size();
			Expression::Alloc(buffer, sizeof(unsigned int));
			size_t start = buffer.size();
			ConfigureDrawItems(aId, element, buffer);
			*new (buffer.data() + buffer_size_at) unsigned int = unsigned int(buffer.size() - start);
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

			Expression::Append(buffer, DO_Emitter, count, period, x, y, a);

			size_t buffer_size_at = buffer.size();
			Expression::Alloc(buffer, sizeof(unsigned int));
			size_t start = buffer.size();
			ConfigureDrawItems(aId, element, buffer);
			*new (buffer.data() + buffer_size_at) unsigned int = unsigned int(buffer.size() - start);
		}
		break;
#endif

		//
		// VARIABLE COMMANDS

	case 0xc6270703 /* "set" */:
		{
			ConfigureVariableOperator(element, buffer, VariableOperatorSet);
		}
		break;

	case 0x3b391274 /* "add" */:
		{
			ConfigureVariableOperator(element, buffer, VariableOperatorAdd);
		}
		break;

	case 0xdc4e3915 /* "sub" */:
		{
			ConfigureVariableOperator(element, buffer, VariableOperatorSub);
		}
		break;

	case 0xeb84ed81 /* "mul" */:
		{
			ConfigureVariableOperator(element, buffer, VariableOperatorMul);
		}
		break;

	case 0xe562ab48 /* "div" */:
		{
			ConfigureVariableOperator(element, buffer, VariableOperatorDiv);
		}
		break;

	case 0xc98f4557 /* "min" */:
		{
			ConfigureVariableOperator(element, buffer, VariableOperatorMin);
		}
		break;

	case 0xd7a2e319 /* "max" */:
		{
			ConfigureVariableOperator(element, buffer, VariableOperatorMax);
		}
		break;

	case 0x3deb1461 /* "swizzle" */:
		{
			const unsigned int name = Hash(element->Attribute("name"));
			const unsigned int type = Hash(element->Attribute("type"));
			int width;
			const char * const *names;
			const float *data;
			GetTypeData(type, width, names, data);

			Expression::Append(buffer, DO_Swizzle, name, width);

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
			ConfigureVariableClear(element, buffer);
		}
		break;

	default:
		DebugPrint("Unknown draw item \"%s\"\n", element->Value());
		break;
	}
}

void ConfigureDrawItems(unsigned int aId, const tinyxml2::XMLElement *element, std::vector<unsigned int> &buffer)
{
	// process child elements
	for (const tinyxml2::XMLElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
	{
		ConfigureDrawItem(aId, child, buffer);
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
void ExecuteDrawItems(EntityContext &aContext)
{
	while (aContext.mStream < aContext.mEnd)
		Expression::Evaluate<void>(aContext);
}
#pragma optimize( "", on )

void InitDrawlists(void)
{
}

void CleanupDrawlists(void)
{
	// for each entry in the drawlist database...
	for (Database::Typed<GLuint>::Iterator itor(&Database::drawlist); itor.IsValid(); ++itor)
	{
		// if the handle is still valid...
		GLuint handle = itor.GetValue();
		if (glIsList(handle))
		{
			// delete the draw list
			glDeleteLists(handle, 1);
		}
	}
}

void RebuildDrawlists(void)
{
	// for each entry in the drawlist database...
	for (Database::Typed<GLuint>::Iterator itor(&Database::drawlist); itor.IsValid(); ++itor)
	{
		// recreate the draw list
		GLuint handle = itor.GetValue();
		glNewList(handle, GL_COMPILE);

		// process draw items
		// TO DO: recover id value
		const std::vector<unsigned int> &drawlist = Database::dynamicdrawlist.Get(handle);
		if (drawlist.size() > 0)
		{
			EntityContext context(&drawlist[0], drawlist.size(), 0.0f, 0);
			ExecuteDrawItems(context);
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
	if (aTransform.p.x != 0.0f || aTransform.p.y != 0.0f)
		glTranslatef(aTransform.p.x, aTransform.p.y, 0);
	if (aTransform.a != 0.0f)
		glRotatef(aTransform.a*180/float(M_PI), 0.0f, 0.0f, 1.0f);

	// execute the deferred draw list
	EntityContext context(&buffer[0], buffer.size(), aTime, aId);
	ExecuteDrawItems(context);

	// reset the transform
	glPopMatrix();
};

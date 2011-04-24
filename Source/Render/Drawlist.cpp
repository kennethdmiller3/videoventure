#include "StdAfx.h"
#include "Drawlist.h"
#include "Variable.h"
#include "Texture.h"
#include "Interpolator.h"
#include "Noise.h"

#include "Expression.h"
#include "ExpressionConfigure.h"


#define DRAWLIST_LOOP


// execute a dynamic draw list
void ExecuteDrawItems(EntityContext &aContext);


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
	case 0xaeebcbdd /* "scalar" */:		names = sScalarNames; data = sScalarDefault; width = sScalarWidth; break;
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
				EntityContext context(&drawlist[0], drawlist.size(), param, aId);
				ExecuteDrawItems(context);

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
bool EvaluateVariableOperator(EntityContext &aContext, VariableOperator op)
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

void DO_glArrayElement(EntityContext &aContext)
{
	glArrayElement(Expression::Read<GLint>(aContext));
}

void DO_glBegin(EntityContext &aContext)
{
	glBegin(Expression::Read<GLenum>(aContext));
}

void DO_glBindTexture(EntityContext &aContext)
{
	GLenum target(Expression::Read<GLenum>(aContext));
	GLuint texture(Expression::Read<GLuint>(aContext));
	glBindTexture(target, texture);
}

void DO_glBlendFunc(EntityContext &aContext)
{
	GLenum sfactor(Expression::Read<GLenum>(aContext));
	GLenum dfactor(Expression::Read<GLenum>(aContext));
	glBlendFunc(sfactor, dfactor);
}

void DO_glCallList(EntityContext &aContext)
{
	glCallList(Expression::Read<GLuint>(aContext));
}

void DO_glColor4fv(EntityContext &aContext)
{
	DLColor value(Expression::Evaluate<DLColor>(aContext));
	glColor4fv(reinterpret_cast<float *>(&value));
}

void DO_glColorPointer(EntityContext &aContext)
{
	GLint size(Expression::Read<GLint>(aContext));
	GLsizei stride(Expression::Read<GLsizei>(aContext));
	size_t count(Expression::Read<size_t>(aContext));
	glColorPointer(size, GL_FLOAT, stride, aContext.mStream);
	aContext.mStream += (count*sizeof(GLfloat)+sizeof(unsigned int)-1)/sizeof(unsigned int);
}

void DO_glDisable(EntityContext &aContext)
{
	glDisable(Expression::Read<GLenum>(aContext));
}

void DO_glDisableClientState(EntityContext &aContext)
{
	glDisableClientState(Expression::Read<GLenum>(aContext));
}

void DO_glDrawArrays(EntityContext &aContext)
{
	GLenum mode(Expression::Read<GLenum>(aContext));
	GLint first(Expression::Read<GLint>(aContext));
	size_t count(Expression::Read<size_t>(aContext));
	glDrawArrays(mode, first, count);
}

void DO_glDrawElements(EntityContext &aContext)
{
	GLenum mode(Expression::Read<GLenum>(aContext));
	size_t count(Expression::Read<size_t>(aContext));
	glDrawElements(mode, count, GL_UNSIGNED_SHORT, aContext.mStream);
	aContext.mStream += (count*sizeof(unsigned short)+sizeof(unsigned int)-1)/sizeof(unsigned int);
}

void DO_glEdgeFlag(EntityContext &aContext)
{
	glEdgeFlag(Expression::Read<GLboolean>(aContext));
}

void DO_glEdgeFlagPointer(EntityContext &aContext)
{
	GLsizei stride(Expression::Read<GLsizei>(aContext));
	size_t count(Expression::Read<size_t>(aContext));
	glEdgeFlagPointer(stride, aContext.mStream);
	aContext.mStream += (count*sizeof(bool)+sizeof(unsigned int)-1)/sizeof(unsigned int);
}

void DO_glEnable(EntityContext &aContext)
{
	glEnable(Expression::Read<GLenum>(aContext));
}

void DO_glEnableClientState(EntityContext &aContext)
{
	glEnableClientState(Expression::Read<GLenum>(aContext));
}

void DO_glEnd(EntityContext &aContext)
{
	glEnd();
}

void DO_glIndexf(EntityContext &aContext)
{
	float value(Expression::Evaluate<DLIndex>(aContext));
	glIndexf(value);
}

void DO_glIndexPointer(EntityContext &aContext)
{
	GLsizei stride(Expression::Read<GLsizei>(aContext));
	size_t count(Expression::Read<size_t>(aContext));
	glIndexPointer(GL_FLOAT, stride, aContext.mStream);
	aContext.mStream += (count*sizeof(GLfloat)+sizeof(unsigned int)-1)/sizeof(unsigned int);
}

void DO_glLineWidth(EntityContext &aContext)
{
	GLfloat width(Expression::Read<GLfloat>(aContext));
	glLineWidth(width);
}

void DO_glLineWidthWorld(EntityContext &aContext)
{
	GLfloat width(Expression::Read<GLfloat>(aContext) * float(SCREEN_HEIGHT) / VIEW_SIZE);
	glLineWidth(width);
}

void DO_glLoadIdentity(EntityContext &aContext)
{
	glLoadIdentity();
}

void DO_glLoadMatrixf(EntityContext &aContext)
{
	glLoadMatrixf(reinterpret_cast<const GLfloat *>(aContext.mStream));
	aContext.mStream += (16*sizeof(GLfloat)+sizeof(unsigned int)-1)/sizeof(unsigned int);
}

void DO_glMultMatrixf(EntityContext &aContext)
{
	glMultMatrixf(reinterpret_cast<const GLfloat *>(aContext.mStream));
	aContext.mStream += (16*sizeof(GLfloat)+sizeof(unsigned int)-1)/sizeof(unsigned int);
}

void DO_glNormal3fv(EntityContext &aContext)
{
	DLNormal value(Expression::Evaluate<DLNormal>(aContext));
	glNormal3fv(reinterpret_cast<float *>(&value));
}

void DO_glNormalPointer(EntityContext &aContext)
{
	GLsizei stride(Expression::Read<GLsizei>(aContext));
	size_t count(Expression::Read<size_t>(aContext));
	glNormalPointer(GL_FLOAT, stride, aContext.mStream);
	aContext.mStream += (count*sizeof(GLfloat)+sizeof(unsigned int)-1)/sizeof(unsigned int);
}

void DO_glPointSize(EntityContext &aContext)
{
	GLfloat size(Expression::Read<GLfloat>(aContext));
	glPointSize(size);
}

void DO_glPointSizeWorld(EntityContext &aContext)
{
	GLfloat size(Expression::Read<GLfloat>(aContext) * float(SCREEN_HEIGHT) / VIEW_SIZE);
	glPointSize(size);
}

void DO_glPopAttrib(EntityContext &aContext)
{
	glPopAttrib();
}

void DO_glPopClientAttrib(EntityContext &aContext)
{
	glPopClientAttrib();
}

void DO_glPopMatrix(EntityContext &aContext)
{
	glPopMatrix();
}

void DO_glPushAttrib(EntityContext &aContext)
{
	glPushAttrib(Expression::Read<GLbitfield>(aContext));
}

void DO_glPushClientAttrib(EntityContext &aContext)
{
	glPushClientAttrib(Expression::Read<GLbitfield>(aContext));
}

void DO_glPushMatrix(EntityContext &aContext)
{
	glPushMatrix();
}

void DO_glRotatef(EntityContext &aContext)
{
	float value(Expression::Evaluate<DLRotation>(aContext));
	glRotatef(value, 0, 0, 1);
}

void DO_glScalef(EntityContext &aContext)
{
	DLScale value(Expression::Evaluate<DLScale>(aContext));
	glScalef(reinterpret_cast<float *>(&value)[0], reinterpret_cast<float *>(&value)[1], reinterpret_cast<float *>(&value)[2]);
}

void DO_glTexCoord2fv(EntityContext &aContext)
{
	DLTexCoord value(Expression::Evaluate<DLTexCoord>(aContext));
	glTexCoord2fv(reinterpret_cast<float *>(&value));
}

void DO_glTexCoordPointer(EntityContext &aContext)
{
	GLint size(Expression::Read<GLint>(aContext));
	GLsizei stride(Expression::Read<GLsizei>(aContext));
	size_t count(Expression::Read<size_t>(aContext));
	glTexCoordPointer(size, GL_FLOAT, stride, aContext.mStream);
	aContext.mStream += (count*sizeof(GLfloat)+sizeof(unsigned int)-1)/sizeof(unsigned int);
}

void DO_glTexEnvi(EntityContext &aContext)
{
	GLenum pname(Expression::Read<GLint>(aContext));
	GLint param(Expression::Read<GLint>(aContext));
	glTexEnvi( GL_TEXTURE_ENV, pname, param );
}

void DO_glTranslatef(EntityContext &aContext)
{
	DLTranslation value(Expression::Evaluate<DLTranslation>(aContext));
	glTranslatef(reinterpret_cast<float *>(&value)[0], reinterpret_cast<float *>(&value)[1], reinterpret_cast<float *>(&value)[2]);
}

void DO_glVertex3fv(EntityContext &aContext)
{
	DLPosition value(Expression::Evaluate<DLPosition>(aContext));
	glVertex3fv(reinterpret_cast<float *>(&value));
}

void DO_glVertexPointer(EntityContext &aContext)
{
	GLint size(Expression::Read<GLint>(aContext));
	GLsizei stride(Expression::Read<GLsizei>(aContext));
	size_t count(Expression::Read<size_t>(aContext));
	glVertexPointer(size, GL_FLOAT, stride, aContext.mStream);
	aContext.mStream += (count*sizeof(GLfloat)+sizeof(unsigned int)-1)/sizeof(unsigned int);
}

void DO_Repeat(EntityContext &aContext)
{
	int repeat(Expression::Read<int>(aContext));
	size_t size(Expression::Read<size_t>(aContext));
	EntityContext context(aContext);
	context.mBegin = context.mStream;
	context.mEnd = context.mStream + size;
	for (int i = 0; i < repeat; i++)
	{
		context.mStream = context.mBegin;
		ExecuteDrawItems(context);
	}
	aContext.mStream += size;
}

void DO_Block(EntityContext &aContext)
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
			EntityContext context(aContext);
			context.mBegin = context.mStream;
			context.mEnd = context.mStream + size;
			context.mParam = t;
			ExecuteDrawItems(context);
		}
	}
	aContext.mStream += size;
}

void DO_Set(EntityContext &aContext)
{
	EvaluateVariableOperator(aContext, VariableOperatorSet);
}

void DO_Add(EntityContext &aContext)
{
	EvaluateVariableOperator(aContext, VariableOperatorAdd);
}

void DO_Sub(EntityContext &aContext)
{
	EvaluateVariableOperator(aContext, VariableOperatorSub);
}

void DO_Mul(EntityContext &aContext)
{
	EvaluateVariableOperator(aContext, VariableOperatorMul);
}

void DO_Div(EntityContext &aContext)
{
	EvaluateVariableOperator(aContext, VariableOperatorDiv);
}

void DO_Min(EntityContext &aContext)
{
	EvaluateVariableOperator(aContext, VariableOperatorMin);
}

void DO_Max(EntityContext &aContext)
{
	EvaluateVariableOperator(aContext, VariableOperatorMax);
}

void DO_Swizzle(EntityContext &aContext)
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

void DO_Clear(EntityContext &aContext)
{
	unsigned int name(Expression::Read<unsigned int>(aContext));
	int width(Expression::Read<int>(aContext));
//	Database::Typed<float> &variables = Database::variable.Open(aContext.mId);
	for (int i = 0; i < width; i++)
		aContext.mVars->Delete(name+i);
//	Database::variable.Close(aContext.mId);
}

#ifdef DRAWLIST_LOOP
void DO_Loop(EntityContext &aContext)
{
	unsigned int name = Expression::Read<unsigned int>(aContext);
	float from = Expression::Read<float>(aContext);
	float to   = Expression::Read<float>(aContext);
	float by   = Expression::Read<float>(aContext);
	size_t size = Expression::Read<size_t>(aContext);

//		Database::Typed<float> &variables = Database::variable.Open(aContext.mId);
	EntityContext context(aContext);
	context.mBegin = context.mStream;
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
		Expression::Append(buffer, value);
		item = strtok(NULL, " \t\n\r,;");
	}
}

void ConfigureVariableOperator(const TiXmlElement *element, std::vector<unsigned int> &buffer, void (*op)(EntityContext &), bool drawdata)
{
	unsigned int name = Hash(element->Attribute("name"));
	unsigned int type = Hash(element->Attribute("type"));
	int width;
	const char * const *names;
	const float *data;
	GetTypeData(type, width, names, data);

	Expression::Append(buffer, op, name, width);
	if (drawdata)
	{
		switch (width)
		{
		case 1: Expression::Loader<float>::ConfigureRoot(element, buffer, names, data); break;
		case 2: //Expression::Loader<Vector2>::ConfigureRoot(element, buffer, names, data); break;
		case 3: //Expression::Loader<Vector3>::ConfigureRoot(element, buffer, names, data); break;
		case 4: Expression::Loader<__m128>::ConfigureRoot(element, buffer, names, data); break;
		}
	}
}

void ConfigurePrimitive(const TiXmlElement *element, std::vector<unsigned int> &buffer, GLenum mode)
{
	Expression::Append(buffer, DO_glBegin, mode);
	ConfigureDrawItems(element, buffer);
	Expression::Append(buffer, DO_glEnd);
}

void ConfigureArray(const TiXmlElement *element, std::vector<unsigned int> &buffer, void (*op)(EntityContext &), size_t size, size_t stride)
{
	Expression::Append(buffer, op, size, stride);

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
			Expression::Loader<DLTranslation>::ConfigureRoot(element, buffer, sPositionNames, sPositionDefault);
		}
		break;

	case 0xa5f4fd0a /* "rotate" */:
		{
			Expression::Append(buffer, DO_glRotatef);
			Expression::Loader<DLRotation>::ConfigureRoot(element, buffer, sRotationNames, sRotationDefault);
		}
		break;

	case 0x82971c71 /* "scale" */:
		{
			Expression::Append(buffer, DO_glScalef);
			Expression::Loader<DLScale>::ConfigureRoot(element, buffer, sScaleNames, sScaleDefault);
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
				Expression::Append(buffer, m);
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
				Expression::Append(buffer, m);
			}
		}
		break;

	case 0x945367a7 /* "vertex" */:
		{
			Expression::Append(buffer, DO_glVertex3fv);
			Expression::Loader<DLPosition>::ConfigureRoot(element, buffer, sPositionNames, sPositionDefault);
		}
		break;

	case 0xe68b9c52 /* "normal" */:
		{
			Expression::Append(buffer, DO_glNormal3fv);
			Expression::Loader<DLNormal>::ConfigureRoot(element, buffer, sPositionNames, sPositionDefault);
		}
		break;

	case 0x3d7e6258 /* "color" */:
		{
			Expression::Append(buffer, DO_glColor4fv);
			Expression::Loader<DLColor>::ConfigureRoot(element, buffer, sColorNames, sColorDefault);
		}
		break;

	case 0x090aa9ab /* "index" */:
		{
			Expression::Append(buffer, DO_glIndexf);
			Expression::Loader<DLIndex>::ConfigureRoot(element, buffer, sIndexNames, sIndexDefault);
		}
		break;

	case 0xdd612dd3 /* "texcoord" */:
		{
			Expression::Append(buffer, DO_glTexCoord2fv);
			Expression::Loader<DLTexCoord>::ConfigureRoot(element, buffer, sTexCoordNames, sTexCoordDefault);
		}
		break;

	case 0x0135ab46 /* "edgeflag" */:
		{
			int flag;
			if (element->QueryIntAttribute("flag", &flag) == TIXML_SUCCESS)
			{
				Expression::Append(buffer, DO_glEdgeFlag, flag ? GL_TRUE : GL_FALSE);
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
					Expression::Append(buffer, DO_glEnable, GL_TEXTURE_2D);
					Expression::Append(buffer, DO_glBindTexture, GL_TEXTURE_2D, texture);
				}
			}
		}

	case 0x059e3a91 /* "texenv" */:
		{
			// set blend mode
			switch (Hash(element->Attribute("mode")))
			{
			case 0x818f75ae /* "modulate" */:	Expression::Append(buffer, DO_glTexEnvi, GL_TEXTURE_ENV_MODE, GL_MODULATE); break;
			case 0xde15f6ae /* "decal" */:		Expression::Append(buffer, DO_glTexEnvi, GL_TEXTURE_ENV_MODE, GL_DECAL); break;
			case 0x0bbc40d8 /* "blend" */:		Expression::Append(buffer, DO_glTexEnvi, GL_TEXTURE_ENV_MODE, GL_BLEND); break;
			case 0xa13884c3 /* "replace" */:	Expression::Append(buffer, DO_glTexEnvi, GL_TEXTURE_ENV_MODE, GL_REPLACE); break;
			}
		}
		break;

	case 0xbc9567c6 /* "points" */:
		{
			float size = 0.0f;
			element->QueryFloatAttribute("size", &size);
			if (size != 0.0f)
			{
				Expression::Append(buffer, DO_glPushAttrib, GL_POINT_BIT);
				Expression::Append(buffer, DO_glPointSizeWorld, size);
			}
			ConfigurePrimitive(element, buffer, GL_POINTS);
			if (size != 0.0f)
			{
				Expression::Append(buffer, DO_glPopAttrib);
			}
		}
		break;

	case 0xe1e4263c /* "lines" */:
		{
			float width = 0.0f;
			element->QueryFloatAttribute("width", &width);
			if (width != 0.0f)
			{
				Expression::Append(buffer, DO_glPushAttrib, GL_LINE_BIT);
				Expression::Append(buffer, DO_glLineWidthWorld, width);
			}
			ConfigurePrimitive(element, buffer, GL_LINES);
			if (width != 0.0f)
			{
				Expression::Append(buffer, DO_glPopAttrib);
			}
		}
		break;

	case 0xc2106ab6 /* "line_loop" */:
		{
			float width = 0.0f;
			element->QueryFloatAttribute("width", &width);
			if (width != 0.0f)
			{
				Expression::Append(buffer, DO_glPushAttrib, GL_LINE_BIT);
				Expression::Append(buffer, DO_glLineWidthWorld, width);
			}
			ConfigurePrimitive(element, buffer, GL_LINE_LOOP);
			if (width != 0.0f)
			{
				Expression::Append(buffer, DO_glPopAttrib);
			}
		}
		break;

	case 0xc6f2fa0e /* "line_strip" */:
		{
			float width = 0.0f;
			element->QueryFloatAttribute("width", &width);
			if (width != 0.0f)
			{
				Expression::Append(buffer, DO_glPushAttrib, GL_LINE_BIT);
				Expression::Append(buffer, DO_glLineWidthWorld, width);
			}
			ConfigurePrimitive(element, buffer, GL_LINE_STRIP);
			if (width != 0.0f)
			{
				Expression::Append(buffer, DO_glPopAttrib);
			}
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

			Expression::Append(buffer, DO_glBlendFunc, srcfactor, dstfactor);
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
					Expression::Append(buffer, DO_glCallList, drawlist);
				}
				else
				{
					DebugPrint("Missing drawlist %s\n", name);
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
			EntityContext context(&drawlist[0], drawlist.size(), param, 0);
			ExecuteDrawItems(context);

			// close the dynamic draw list
			Database::dynamicdrawlist.Close(handle);

			// finish the draw list
			glEndList();

			// use the anonymous drawlist
			Expression::Append(buffer, DO_glCallList, handle);
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

			Expression::Append(buffer, DO_glEdgeFlagPointer, stride, count);
			for (size_t i = 0; i < count+sizeof(unsigned int)/sizeof(bool)-1; i += sizeof(unsigned int)/sizeof(bool))
				buffer.push_back(*reinterpret_cast<unsigned int *>(&data[i]));
		}
		break;

	case 0x0a85bb5e /* "arrayelement" */:
		{
			int index;
			if (element->QueryIntAttribute("index", &index) == TIXML_SUCCESS)
			{
				Expression::Append(buffer, DO_glArrayElement, index);
			}
		}
		break;

	case 0xf4de4a21 /* "drawarrays" */:
		{
			GLenum mode(GetPrimitiveMode(element->Attribute("mode")));

			int first = 0, count = 0;
			element->QueryIntAttribute("first", &first);
			element->QueryIntAttribute("count", &count);
			Expression::Append(buffer, DO_glDrawArrays, mode, first, count);
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

			Expression::Append(buffer, DO_glDrawElements, mode, count);
			for (size_t i = 0; i < count+sizeof(unsigned int)/sizeof(unsigned short)-1; i += sizeof(unsigned int)/sizeof(unsigned short))
				buffer.push_back(*reinterpret_cast<unsigned int *>(&indices[i]));
		}
		break;

	case 0xd99ba82a /* "repeat" */:
		{
			int count = 1;
			element->QueryIntAttribute("count", &count);

			Expression::Append(buffer, DO_Repeat, count);

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

			Expression::Append(buffer, DO_Block, start, length, scale, repeat);

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

			Expression::Append(buffer, DO_Loop, name, from, to, by);

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

			Expression::Append(buffer, DO_Emitter, count, period, x, y, a);

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
void ExecuteDrawItems(EntityContext &aContext)
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
			EntityContext context(&drawlist[0], drawlist.size(), param, 0);
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

#if 0
	// push a transform
	glPushMatrix();

	// load matrix
	glTranslatef(aTransform.p.x, aTransform.p.y, 0);
	glRotatef(aTransform.a*180/float(M_PI), 0.0f, 0.0f, 1.0f);
#endif

	// execute the deferred draw list
	EntityContext context(&buffer[0], buffer.size(), aTime, aId);
	ExecuteDrawItems(context);

#if 0
	// reset the transform
	glPopMatrix();
#endif
};

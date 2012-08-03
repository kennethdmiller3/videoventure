#include "StdAfx.h"

#include "ExpressionAction.h"

#include "ExpressionConfigure.h"
#include "ExpressionEntity.h"
#include "Resource.h"
#include "Entity.h"


static const char * const sTransformNames[] = { "x", "y", "angle", "" };
static const float sTransformDefault[] = { 0.0f, 0.0f, 0.0f, 0.0f };

template<> inline Transform2 Cast<Transform2, __m128>(__m128 i)
{
	return Transform2(i.m128_f32[2], Vector2(i.m128_f32[0], i.m128_f32[1]));
}


void Expression::Spawn(EntityContext &aContext)
{
	// hacktastic!
	unsigned int id(Expression::Read<unsigned int>(aContext));
	Transform2 offset(Cast<Transform2, __m128>(Expression::Evaluate<__m128>(aContext)));
	Transform2 velocity(Cast<Transform2, __m128>(Expression::Evaluate<__m128>(aContext)));
	Entity *entity = Database::entity.Get(aContext.mId);
	offset = entity->GetTransform() * offset;
	velocity.p = entity->GetTransform().Rotate(velocity.p);
	Database::Instantiate(id, Database::owner.Get(aContext.mId), aContext.mId, offset.a, offset.p, velocity.p, velocity.a, true);
}

void Expression::Switch(EntityContext &aContext)
{
	unsigned int id(Expression::Read<unsigned int>(aContext));
	Database::Switch(aContext.mId, id);
}

void Expression::AddResource(EntityContext &aContext)
{
	unsigned int name(Expression::Read<unsigned int>(aContext));
	float value(Expression::Evaluate<float>(aContext));
	unsigned int id = FindResourceContainer(aContext.mId, name);
	if (Resource *resource = Database::resource.Get(id).Get(name))
		resource->Add(aContext.mId, value);
}

void Expression::Repeat(EntityContext &aContext)
{
	int repeat(Expression::Read<int>(aContext));
	size_t size(Expression::Read<size_t>(aContext));
	EntityContext context(aContext.mStream, size, aContext.mParam, aContext.mId);
	for (int i = 0; i < repeat; ++i, context.Restart())
	{
		Expression::Evaluate<void>(context);
	}
	aContext.mStream += size;
}

void Expression::Loop(EntityContext &aContext)
{
	unsigned int name = Expression::Read<unsigned int>(aContext);
	float from = Expression::Read<float>(aContext);
	float to   = Expression::Read<float>(aContext);
	float by   = Expression::Read<float>(aContext);
	size_t size = Expression::Read<size_t>(aContext);

	EntityContext context(aContext.mStream, size, aContext.mParam, aContext.mId);
	if (by > 0)
	{
		for (float value = from; value <= to; value += by)
		{
			context.mVars->Put(name, value);
			context.mStream = aContext.mStream;
			Expression::Evaluate<void>(context);
		}
	}
	else
	{
		for (float value = from; value >= to; value += by)
		{
			context.mVars->Put(name, value);
			context.mStream = aContext.mStream;
			Expression::Evaluate<void>(context);
		}
	}
	context.mVars->Delete(name);

	aContext.mStream += size;
}


static void ConfigureActionItem(const tinyxml2::XMLElement *element, std::vector<unsigned int> &buffer)
{
	switch (Hash(element->Value()))
	{
	case 0x3a224d98 /* "spawn" */:
		{
			Expression::Append(buffer, Expression::Spawn, Hash(element->Attribute("name")));
			if (const tinyxml2::XMLElement *param = element->FirstChildElement("offset"))
				Expression::Loader<__m128>::ConfigureRoot(param, buffer, sTransformNames, sTransformDefault);
			else
				Expression::Append(buffer, Expression::Read<__m128>, _mm_setzero_ps());
			if (const tinyxml2::XMLElement *param = element->FirstChildElement("velocity"))
				Expression::Loader<__m128>::ConfigureRoot(param, buffer, sTransformNames, sTransformDefault);
			else
				Expression::Append(buffer, Expression::Read<__m128>, _mm_setzero_ps());
		}
		break;

	case 0x93e05f71 /* "switch" */:
		{
			Expression::Append(buffer, Expression::Switch, Hash(element->Attribute("name")));
		}
		break;

	case 0xfd5e91a8 /* "addresource" */:
		{
			Expression::Append(buffer, Expression::AddResource, Hash(element->Attribute("name")));
			Expression::Loader<float>::ConfigureRoot(element, buffer, sScalarNames, sScalarDefault);
		}
		break;

	case 0xd99ba82a /* "repeat" */:
		{
			int count = 1;
			element->QueryIntAttribute("count", &count);

			Expression::Append(buffer, Expression::Repeat, count);

			buffer.push_back(0);
			int start = buffer.size();
			ConfigureAction(element, buffer);
			buffer[start-1] = buffer.size() - start;
		}
		break;

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

			Expression::Append(buffer, Expression::Loop, name, from, to, by);

			buffer.push_back(0);
			int start = buffer.size();
			ConfigureAction(element, buffer);
			buffer[start-1] = buffer.size() - start;
		}
		break;
	}
}


void ConfigureAction(const tinyxml2::XMLElement *element, std::vector<unsigned int> &buffer)
{
	for (const tinyxml2::XMLElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
	{
		ConfigureActionItem(child, buffer);
	}
}

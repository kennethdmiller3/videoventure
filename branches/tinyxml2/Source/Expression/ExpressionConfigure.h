#pragma once

#include "ExpressionLiteral.h"
#include "ExpressionVariable.h"
#include "ExpressionInterpolator.h"
#include "ExpressionRandom.h"
#include "ExpressionConstruct.h"

extern GAME_API const char * const sScalarNames[];
extern GAME_API const float sScalarDefault[];

//
// EXPRESSION


namespace Expression
{
	typedef fastdelegate::FastDelegate<void (const tinyxml2::XMLElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[])> Entry;

	template <typename T> struct Loader
	{
		static Database::Typed<Entry> &GetDB()
		{
			static Database::Typed<Entry> configure;
			return configure;
		}
		static void Add(unsigned int aTagId, Entry aConfigure)
		{
			GetDB().Put(aTagId, aConfigure);
		}
		static const Entry &Get(unsigned int aTagId)
		{
			return GetDB().Get(aTagId);
		}

		static struct Auto
		{
			Auto(unsigned int aTagId, Entry aEntry)
			{
				Add(aTagId, aEntry);
			}
		};

		// configure an expression
		static void GAME_API Configure(const tinyxml2::XMLElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[]);

		// configure an expression root (the tag hosting the expression)
		static void GAME_API ConfigureRoot(const tinyxml2::XMLElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[]);
	};
};

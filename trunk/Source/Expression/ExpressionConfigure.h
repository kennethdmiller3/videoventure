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
	typedef void (*Entry)(const tinyxml2::XMLElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[]);

	template <typename T> class Loader
	{
	private:
		unsigned int mTagId;
		Entry mPrev;

	public:
		static Database::Typed<Entry> &GetDB();
		Loader(unsigned int aTagId, Entry aEntry);
		~Loader();
		static const Entry &Get(unsigned int aTagId)
		{
			return GetDB().Get(aTagId);
		}

		// configure an expression
		static void GAME_API Configure(const tinyxml2::XMLElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[]);

		// configure an expression root (the tag hosting the expression)
		static void GAME_API ConfigureRoot(const tinyxml2::XMLElement *element, std::vector<unsigned int> &buffer, const char * const names[], const float defaults[]);
	};
};

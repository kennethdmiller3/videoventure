#include "StdAfx.h"

#include "Updatable.h"
//#include "ExpressionSchema.h"
#include "ExpressionConfigure.h"
#include "ExpressionAction.h"

class ConditionTemplate
{
public:
	unsigned int mSubId;
	std::vector<unsigned int> mCondition;
	std::vector<unsigned int> mAction;

public:
	bool Configure(const TiXmlElement *element, unsigned int aId, unsigned int aSubId);
};

class Condition : public Updatable
{
public:
	unsigned int mSubId;

public:
	Condition(unsigned int aId, unsigned int aSubId);
	void Update(float aStep);
};

static void UpdateCondition(unsigned int aId);

namespace Database
{
	Typed<Typed<ConditionTemplate> > conditiontemplate(0xb03d9930 /* "conditiontemplate" */);
	Typed<Typed<Condition *> > condition(0xd212fabe /* "condition" */);

	namespace Loader
	{
		class ConditionLoader
		{
		public:
			ConditionLoader()
			{
				AddConfigure(0xd212fabe /* "condition" */, Entry(this, &ConditionLoader::Configure));
			}

			void Configure(unsigned int aId, const TiXmlElement *element)
			{
				Database::Typed<ConditionTemplate> &conditions = Database::conditiontemplate.Open(aId);
				unsigned int aSubId = Hash(element->Attribute("name"));
				ConditionTemplate &condition = conditions.Open(aSubId);
				condition.Configure(element, aId, aSubId);
				conditions.Close(aSubId);
				Database::conditiontemplate.Close(aId);
			}
		}
		conditionloader;
	}

	namespace Initializer
	{
		class ConditionInitializer
		{
		public:
			ConditionInitializer()
			{
				AddActivate(0xb03d9930 /* "conditiontemplate" */, Entry(this, &ConditionInitializer::Activate));
				AddDeactivate(0xb03d9930 /* "conditiontemplate" */, Entry(this, &ConditionInitializer::Deactivate));
			}

			void Activate(unsigned int aId)
			{
				Database::Typed<Condition *> &conditions = Database::condition.Open(aId);
				for (Database::Typed<ConditionTemplate>::Iterator itor(Database::conditiontemplate.Find(aId)); itor.IsValid(); ++itor)
				{
					Condition *condition = new Condition(aId, itor.GetKey());
					conditions.Put(itor.GetKey(), condition);
					condition->Activate();
				}
				Database::condition.Close(aId);
			}

			void Deactivate(unsigned int aId)
			{
				for (Database::Typed<Condition *>::Iterator itor(Database::condition.Find(aId)); itor.IsValid(); ++itor)
				{
					delete itor.GetValue();
				}
				Database::condition.Delete(aId);
			}
		}
		conditioninitializer;
	}
}

bool ConditionTemplate::Configure(const TiXmlElement *element, unsigned int aId, unsigned int aSubId)
{
	for (const TiXmlElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
	{
		switch (Hash(child->Value()))
		{
		case 0xd212fabe /* "condition" */:
			Expression::Loader<bool>::Configure(child->FirstChildElement(), mCondition, sScalarNames, sScalarDefault);
			break;

		case 0xc4642eff /* "action" */:
			ConfigureAction(child->FirstChildElement(), mAction);
			break;
		}
	}
	return true;
}


Condition::Condition(unsigned int aId, unsigned int aSubId)
: Updatable(aId)
, mSubId(aSubId)
{
	SetAction(Action(this, &Condition::Update));
}

void Condition::Update(float aStep)
{
	const ConditionTemplate &conditiontemplate = Database::conditiontemplate.Get(mId).Get(mSubId);
	
	if (!conditiontemplate.mCondition.empty())
	{
		EntityContext context(&conditiontemplate.mCondition[0], conditiontemplate.mCondition.size(), sim_turn * sim_step, mId);
		bool result = true;
		while (context.mStream < context.mEnd)
		{
			if (!Expression::Evaluate<bool>(context))
			{
				result = false;
				break;
			}
		}
		if (!result)
			return;
	}

	if (!conditiontemplate.mAction.empty())	
	{
		EntityContext context(&conditiontemplate.mAction[0], conditiontemplate.mAction.size(), sim_turn * sim_step, mId);
		while (context.mStream < context.mEnd)
			Expression::Evaluate<void>(context);
	}
}

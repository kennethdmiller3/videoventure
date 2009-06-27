#include "StdAfx.h"

#include "PathBehavior.h"
#include "TargetBehavior.h"
#include "BotUtilities.h"
#include "Controller.h"
#include "Entity.h"

namespace Database
{
	Typed<PathBehaviorTemplate> pathbehaviortemplate(0x523d9dba /* "pathbehaviortemplate" */);
	Typed<PathBehavior *> pathbehavior(0x0ad20210 /* "pathbehavior" */);
}

namespace BehaviorDatabase
{
	namespace Loader
	{
		class PathBehaviorLoader
		{
		public:
			PathBehaviorLoader()
			{
				AddConfigure(0x84874d36 /* "path" */, Entry(this, &PathBehaviorLoader::Configure));
			}

			unsigned int Configure(unsigned int aId, const TiXmlElement *element)
			{
				PathBehaviorTemplate &path = Database::pathbehaviortemplate.Open(aId);
				path.Configure(element, aId);
				Database::pathbehaviortemplate.Close(aId);
				return 0x523d9dba /* "pathbehaviortemplate" */;
			}
		}
		pathbehaviorloader;
	}

	namespace Initializer
	{
		class PathBehaviorInitializer
		{
		public:
			PathBehaviorInitializer()
			{
				AddActivate(0x523d9dba /* "pathbehaviortemplate" */, ActivateEntry(this, &PathBehaviorInitializer::Activate));
				AddDeactivate(0x523d9dba /* "pathbehaviortemplate" */, DeactivateEntry(this, &PathBehaviorInitializer::Deactivate));
			}

			Behavior *Activate(unsigned int aId, Controller *aController)
			{
				const PathBehaviorTemplate &pathbehaviortemplate = Database::pathbehaviortemplate.Get(aId);
				PathBehavior *pathbehavior = new PathBehavior(aId, pathbehaviortemplate, aController);
				Database::pathbehavior.Put(aId, pathbehavior);
				return pathbehavior;
			}

			void Deactivate(unsigned int aId)
			{
				if (PathBehavior *pathbehavior = Database::pathbehavior.Get(aId))
				{
					delete pathbehavior;
					Database::pathbehavior.Delete(aId);
				}
			}
		}
		pathbehaviorinitializer;
	}
}

PathBehaviorTemplate::PathBehaviorTemplate()
: mStrength(0.0f)
, mLeading(0.0f)
, mOffset(Transform2::Identity())
{
}

bool PathBehaviorTemplate::Configure(const TiXmlElement *element, unsigned int aId)
{
	element->QueryFloatAttribute("strength", &mStrength);
	element->QueryFloatAttribute("leading", &mLeading);
	if (element->QueryFloatAttribute("angle", &mOffset.a) == TIXML_SUCCESS)
		mOffset.a *= float(M_PI) / 180.0f;
	element->QueryFloatAttribute("x", &mOffset.p.x);
	element->QueryFloatAttribute("y", &mOffset.p.y);
	return true;
}

PathBehavior::PathBehavior(unsigned int aId, const PathBehaviorTemplate &aTemplate, Controller *aController)
: Behavior(aId, aController)
{
	bind(this, &PathBehavior::Execute);
}

// path behavior
Status PathBehavior::Execute(void)
{
	// get target
	const TargetData &targetdata = Database::targetdata.Get(mId);

	// get target entity
	Entity *targetEntity = Database::entity.Get(targetdata.mTarget);
	if (!targetEntity)
		return runningTask;

	// get path behavior template
	const PathBehaviorTemplate &path = Database::pathbehaviortemplate.Get(mId);

	// get owner entity
	Entity *entity = Database::entity.Get(mId);

	// direction to target
	Vector2 targetDir(path.mOffset.Untransform(TargetDir(path.mLeading, entity, targetEntity, targetdata.mOffset)));
	
	// save range
	float distSq = targetDir.LengthSq();

	// normalize direction
	targetDir *= InvSqrt(distSq);

	// move towards target
	mController->mMove += path.mStrength * targetDir;

	return runningTask;
}

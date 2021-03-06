#include "StdAfx.h"
#include "Entity.h"
#include "Link.h"
#include "Collidable.h"

struct Tile
{
	unsigned int mSpawn;
	Transform2 mOffset;
};

class TilemapTemplate
{
public:
	Tile *mMap;
	size_t mCount;

public:
	TilemapTemplate(void)
		: mMap(NULL)
	{
	}

	~TilemapTemplate()
	{
		free(mMap);
	}

	void Add(unsigned int aSpawn, const Transform2 &aOffset)
	{
		mMap = static_cast<Tile *>(realloc(mMap, (mCount + 1) * sizeof(Tile)));
		mMap[mCount].mSpawn = aSpawn;
		mMap[mCount].mOffset = aOffset;
		++mCount;
	}
};

class Tilemap
{
public:
	unsigned int mId;
	unsigned int *mInstance;
	size_t mCount;

	Tilemap(void)
		: mId(0)
		, mInstance(NULL)
		, mCount(0)
	{
	}

	Tilemap(const TilemapTemplate &aTemplate, unsigned int aId)
		: mId(aId)
	{
		Entity *entity = Database::entity.Get(aId);
		Transform2 transform = entity ? entity->GetTransform() : Transform2::Identity();

		mInstance = static_cast<unsigned int *>(malloc(aTemplate.mCount * sizeof(unsigned int)));
		mCount = aTemplate.mCount;

		for (size_t i = 0; i < mCount; ++i)
		{
			const Tile &tile = aTemplate.mMap[i];
			const Transform2 spawn(tile.mOffset * transform);

			mInstance[i] = Database::Instantiate(tile.mSpawn, mId, mId, spawn.a, spawn.p);

			if (mId)
			{
				// link it (HACK)
				Database::Typed<LinkTemplate> &linktemplates = Database::linktemplate.Open(mId);
				LinkTemplate &linktemplate = linktemplates.Open(mInstance[i]);
				linktemplate.mOffset = tile.mOffset;
				linktemplate.mSub = mInstance[i];
				linktemplate.mSecondary = mInstance[i];
				Link *link = new Link(linktemplate, mId);
				linktemplates.Close(mInstance[i]);
				Database::linktemplate.Close(mId);
				Database::Typed<Link *> &links = Database::link.Open(mId);
				links.Put(mInstance[i], link);
				Database::link.Close(mId);

#if 0
				// if linking two collidables
				if (Database::collidabletemplate.Find(mId) &&
					Database::collidabletemplate.Find(mInstance[i]))
				{
					// if updating position
					if (linktemplate.mUpdatePosition)
					{
						// add a revolute joint to the linked template (HACK)
						CollidableTemplate &collidable = Database::collidabletemplate.Open(mInstance[i]);
						collidable.SetupLinkJoint(linktemplate, aId, mInstance[i]);
						Database::collidabletemplate.Close(mInstance[i]);
					}
				}
				// else if updating angle or position...
				else if (linktemplate.mUpdateAngle || linktemplate.mUpdatePosition)
#endif
				{
					// activate link update
					link->Activate();
				}
			}
		}
	}

	~Tilemap()
	{
		for (size_t i = 0; i < mCount; ++i)
		{
			Database::Delete(mInstance[i]);
		}

		free(mInstance);
	}
};

namespace Database
{
	Database::Typed<TilemapTemplate> tilemaptemplate(0x059c7d0b /* "tilemaptemplate" */);
	Database::Typed<Tilemap *> tilemap(0xbaf310c5 /* "tilemap" */);

	namespace Loader
	{
		static void TilemapConfigure(unsigned int aId, const tinyxml2::XMLElement *element)
		{
			// tilemap configuration
			float x = 0.0f, y = 0.0f;
			element->QueryFloatAttribute("x", &x);
			element->QueryFloatAttribute("y", &y);
			float dx = 1.0f, dy = 1.0f;
			element->QueryFloatAttribute("dx", &dx);
			element->QueryFloatAttribute("dy", &dy);

			// tiles
			struct Tile
			{
				unsigned int mSpawn;
				Transform2 mOffset;
			};
			Tile map[CHAR_MAX-CHAR_MIN+1];
			memset(map, 0, sizeof(map));

			// position value
			Vector2 pos(x, y);

			// get the tilemap template
			TilemapTemplate &tilemap = Database::tilemaptemplate.Open(aId);

			// process child elements
			for (const tinyxml2::XMLElement *child = element->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
			{
				switch(Hash(child->Value()))
				{
				case 0x713a7cc9 /* "tile" */:
					{
						const char *name = child->Attribute("name");
						if (!name || !name[0])
							continue;
						Tile &tile = map[name[0]-CHAR_MIN];
						const char *spawn = child->Attribute("spawn");
						tile.mSpawn = Hash(spawn);
						child->QueryFloatAttribute("x", &tile.mOffset.p.x);
						child->QueryFloatAttribute("y", &tile.mOffset.p.y);
						if (child->QueryFloatAttribute("angle", &tile.mOffset.a) == tinyxml2::XML_SUCCESS)
							tile.mOffset.a *= float(M_PI) / 180.0f;
					}
					break;

				case 0x440e1d7b /* "row" */:
					{
						pos.x = x;

						const char *text = child->Attribute("data");
						if (!text)
							text = child->GetText();
						if (!text)
							continue;

						for (const char *t = text; *t; ++t)
						{
							Tile &tile = map[*t-CHAR_MIN];
							if (tile.mSpawn)
							{
								Transform2 transform(tile.mOffset * Transform2(0, pos));
								if (aId)
									tilemap.Add(tile.mSpawn, transform);
								else
									Database::Instantiate(tile.mSpawn, 0, 0, transform.a, transform.p);
							}

							pos.x += dx;
						}

						pos.y += dy;
					}
					break;

				default:
					break;
				}
			}

			Database::tilemaptemplate.Close(aId);
		}
		Configure tilemapconfigure(0xbaf310c5 /* "tilemap" */, TilemapConfigure);
	}

	namespace Initializer
	{
		static void TilemapActivate(unsigned int aId)
		{
			const TilemapTemplate &tilemaptemplate = Database::tilemaptemplate.Get(aId);
			Tilemap *tilemap = new Tilemap(tilemaptemplate, aId);
			Database::tilemap.Put(aId, tilemap);
		}
		Activate tilemapactivate(0x059c7d0b /* "tilemaptemplate" */, TilemapActivate);

		static void TilemapDeactivate(unsigned int aId)
		{
			if (Tilemap *tilemap = Database::tilemap.Get(aId))
			{
				delete tilemap;
				Database::tilemap.Delete(aId);
			}
		}
		Deactivate tilemapdeactivate(0x059c7d0b /* "tilemaptemplate" */, TilemapDeactivate);
	}
}
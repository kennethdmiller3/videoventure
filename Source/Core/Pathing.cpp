#include "StdAfx.h"
#include "Pathing.h"
#include "Collidable.h"

#include <deque>
#include <map>
#include <algorithm>

/*
function A*(start,goal)
     closedlist := the empty set               % The set of nodes already evaluated.
     openlist := set containing the start node % The set of tentative nodes to be evaluated.
     g_score[start] := 0                       % Distance from start along optimal path.
     while openlist is not empty
         x := the node in openlist having the lowest f_score[] value
         if x = goal
             return path traced through came_from[]
         remove x from openlist
         add x to closedlist
         foreach y in neighbor_nodes(x)
             if y in closedlist
                 continue
             tentative_g_score := g_score[x] + dist_between(x,y)
             tentative_is_better := false
             if y not in openlist
                 add y to openlist
                 h_score[y] := estimated_distance_to_goal(y)
                 tentative_is_better := true
             elsif tentative_g_score < g_score[y]
                 tentative_is_better := true
             if tentative_is_better = true
                 came_from[y] := x
                 g_score[y] := tentative_g_score
                 f_score[y] := g_score[y] + h_score[y] % Estimated total distance from start to goal through y.
     return failure
*/

static GLuint grid_handle = 0;
Color4 cell_color[] =
{
	Color4( 0.0f, 0.5f, 1.0f, 0.5f ),
	Color4( 1.0f, 0.0f, 0.0f, 1.0f ),
};

static void AddGridSlab(const b2AABB &aabb, Color4 &color)
{
	// trivial accept
	glBegin(GL_QUADS);
	glColor4f(color[0], color[1], color[2], color[3] * 0.5f);
	glVertex2f(aabb.lowerBound.x, aabb.lowerBound.y);
	glVertex2f(aabb.upperBound.x, aabb.lowerBound.y);
	glVertex2f(aabb.upperBound.x, aabb.upperBound.y);
	glVertex2f(aabb.lowerBound.x, aabb.upperBound.y);
	glEnd();
	glBegin(GL_LINE_LOOP);
	glColor4f(color[0], color[1], color[2], color[3]);
	glVertex2f(aabb.lowerBound.x, aabb.lowerBound.y);
	glVertex2f(aabb.upperBound.x, aabb.lowerBound.y);
	glVertex2f(aabb.upperBound.x, aabb.upperBound.y);
	glVertex2f(aabb.lowerBound.x, aabb.upperBound.y);
	glEnd();
}

class GridQueryCallback : public b2QueryCallback
{
public:
	b2Filter mFilter;
	b2Fixture *mBlocker;

public:
	GridQueryCallback(const b2Filter &aFilter)
		: mFilter(aFilter), mBlocker(NULL)
	{
	}

	virtual bool ReportFixture(b2Fixture* fixture)
	{
		// skip unhittable fixtures
		if (fixture->IsSensor())
			return true;
		if (!Collidable::CheckFilter(fixture->GetFilterData(), mFilter))
			return true;

		// get the parent body
		b2Body* body = fixture->GetBody();
		if (body->GetType() != b2_staticBody)
			return true;

		mBlocker = fixture;
		return false;
	}
};

void BuildPathingGrid(const int aZoneSize, const int aCellSize)
{
	if (grid_handle)
	{
		glCallList(grid_handle);
		return;
	}

	// create a new grid handle
	grid_handle = glGenLists(1);

	// create a new list
	glNewList(grid_handle, GL_COMPILE);

	// get world boundary
	const b2AABB &boundary = Collidable::GetBoundary();

	// get zone extents
	int zx0 = xs_FloorToInt(boundary.lowerBound.x / aZoneSize);
	int zx1 = xs_CeilToInt(boundary.upperBound.x / aZoneSize);
	int zy0 = xs_FloorToInt(boundary.lowerBound.y / aZoneSize);
	int zy1 = xs_CeilToInt(boundary.upperBound.y / aZoneSize);

	// cells per zone
	int cell_side = aZoneSize / aCellSize;
	int cell_count = cell_side * cell_side;

	// get the collision world
	b2World *world = Collidable::GetWorld();

	// create a probe fixture
	b2PolygonShape probeshape;
	probeshape.SetAsBox(aCellSize * 0.5f, aCellSize * 0.5f, b2Vec2(aCellSize * 0.5f, aCellSize * 0.5f), 0.0f);
	b2BodyDef probebodydef;
	b2Body *probebody = world->CreateBody(&probebodydef);
	b2FixtureDef probefixturedef;
	probefixturedef.shape = &probeshape;
	probefixturedef.isSensor = true;
	b2Fixture *probe = probebody->CreateFixture(&probefixturedef);

	// for each zone row...
	for (int zy = zy0; zy < zy1; ++zy)
	{
		// for each zone column...
		for (int zx = zx0; zx < zx1; ++zx)
		{
			// get zone boundary
			b2AABB zone_aabb;
			zone_aabb.lowerBound.x = float((zx) * aZoneSize);
			zone_aabb.lowerBound.y = float((zy) * aZoneSize);
			zone_aabb.upperBound.x = float((zx + 1) * aZoneSize);
			zone_aabb.upperBound.y = float((zy + 1) * aZoneSize);

			// draw zone boundary
			glBegin(GL_LINE_LOOP);
			glColor4f(1, 1, 1, 1);
			glVertex2f(zone_aabb.lowerBound.x, zone_aabb.lowerBound.y);
			glVertex2f(zone_aabb.upperBound.x, zone_aabb.lowerBound.y);
			glVertex2f(zone_aabb.upperBound.x, zone_aabb.upperBound.y);
			glVertex2f(zone_aabb.lowerBound.x, zone_aabb.upperBound.y);
			glEnd();

			// initialize cell map
			unsigned char *cell_map = static_cast<unsigned char *>(_alloca(cell_count));
			memset(cell_map, 0xFF, cell_count);

			// for each cell row
			for (int row = 0; row < cell_side; ++row)
			{
				// for each cell column...
				for (int col = 0; col < cell_side; ++col)
				{
					// probe filter
					static const b2Filter aFilter = { 1 << 0, 0xFFFF, 0 };

					// get fixtures intersecting the cell
					b2AABB cell_aabb;
					cell_aabb.lowerBound.x = zone_aabb.lowerBound.x + (col) * aCellSize;
					cell_aabb.lowerBound.y = zone_aabb.lowerBound.y + (row) * aCellSize;
					cell_aabb.upperBound.x = zone_aabb.lowerBound.x + (col + 1) * aCellSize;
					cell_aabb.upperBound.y = zone_aabb.lowerBound.y + (row + 1) * aCellSize;
					GridQueryCallback callback(aFilter);
					world->QueryAABB(&callback, cell_aabb);

					// cell type (0=empty, 1=blocked)
					cell_map[row * cell_side + col] = callback.mBlocker != NULL;
				}
			}

			// initialize slab map
			unsigned char *slab_map = static_cast<unsigned char *>(_alloca(cell_count));
			memset(slab_map, 0xFF, cell_count);
			int slab_count = 0;

			// for each cell row
			for (int row = 0; row < cell_side; ++row)
			{
				// for each cell column...
				for (int col = 0; col < cell_side; ++col)
				{
					// skip assigned spaces
					if (slab_map[row * cell_side + col] != 0xFF)
						continue;

					// cell type
					unsigned char cell = cell_map[row * cell_side + col];

					// allocate a new index
					unsigned char index = unsigned char(slab_count++);
					assert(index < 0xFF);

					// find horizontal extent
					int c0 = col;
					int c1 = cell_side;
					for (int c = c0; c < c1; ++c)
					{
						if ((cell_map[row * cell_side + c] != cell) || ((slab_map[row * cell_side + c] != 0xFF) && (slab_map[row * cell_side + c] != index)))
						{
							c1 = c;
							break;
						}
					}

					// find vertical extent
					int r0 = row;
					int r1 = cell_side;
					for (int r = r0; r < r1; ++r)
					{
						for (int c = c0; c < c1; ++c)
						{
							if ((cell_map[r * cell_side + c] != cell) || ((slab_map[r * cell_side + c] != 0xFF) && (slab_map[r * cell_side + c] != index)))
							{
								r1 = r;
								break;
							}
						}
					}
					
					// fill slab
					for (int r = r0; r < r1; ++r)
					{
						for (int c = c0; c < c1; ++c)
						{
							slab_map[r * cell_side + c] = index;
						}
					}

					assert(c0 < c1 && r0 < r1);

					// set slab extents
					//titleslab[index][0] = c0;
					//titleslab[index][1] = c1;
					//titleslab[index][2] = r0;
					//titleslab[index][3] = r1;
					b2AABB slab_aabb;
					slab_aabb.lowerBound.x = zone_aabb.lowerBound.x + c0 * aCellSize;
					slab_aabb.lowerBound.y = zone_aabb.lowerBound.y + r0 * aCellSize;
					slab_aabb.upperBound.x = zone_aabb.lowerBound.x + c1 * aCellSize;
					slab_aabb.upperBound.y = zone_aabb.lowerBound.y + r1 * aCellSize;
					AddGridSlab(slab_aabb, cell_color[cell]);

					// skip visited columns
					col = c1 - 1;
				}
			}

			DebugPrint("zone %d %d slabs %d\n", zx, zy, slab_count);
		}
	}

	// destroy the probe fixture
	world->DestroyBody(probebody);

	glEndList();
}

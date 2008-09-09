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

/*
bool Pathing_A(const b2Vec2 &start, const b2Vec2 &goal, float radius)
{
	// static state
	static unsigned int statichash = 0;
	static std::map<unsigned int, b2Vec2> closedlist;
	static std::map<unsigned int, b2Vec2> openlist;
	static std::map<unsigned int, float> g_score;
	static std::map<float, unsigned int> f_score;
	static std::map<unsigned int, unsigned int> came_from;

	// node hashes
	unsigned int starthash = Hash(&start, sizeof(start));
	unsigned int goalhash = Hash(&goal, sizeof(goal));

	// get signature
	unsigned int statehash;
	statehash = Hash(&radius, sizeof(radius));
	statehash = Hash(&start, sizeof(start), statehash);
	statehash = Hash(&goal, sizeof(goal), statehash);

	// if the signature changed...
	if (statichash != statehash)
	{
		// update signature
		statichash = statehash;

		// clear static state
		closedlist.clear();	// the set of nodes already evaluated
		openlist.clear();	// the set of tentative nodes to be evaluated
		g_score.clear();	// distance from start along optimal path
		f_score.clear();
		came_from.clear();

		// add start node
		openlist[starthash] = start;
		g_score[starthash] = 0;
	}

	// get the collision world
//	b2World *world = Collidable::GetWorld();

	glBegin(GL_LINES);

	while (!openlist.empty())
	{
		unsigned int x = f_score.begin()->second;				//x := the node in openlist having the lowest f_score[] value
		if (x == goalhash)										//if x = goal
			break;												//    return path traced through came_from[]
		std::pair<unsigned int, b2Vec2> value = *openlist.find(x);
		closedlist.insert(value);								//add x to closedlist
		openlist.erase(value.first);							//remove x from openlist
        //foreach y in neighbor_nodes(x)
        //    if y in closedlist
        //        continue
        //    tentative_g_score := g_score[x] + dist_between(x,y)
        //    tentative_is_better := false
        //    if y not in openlist
        //        add y to openlist
        //        h_score[y] := estimated_distance_to_goal(y)
        //        tentative_is_better := true
        //    elsif tentative_g_score < g_score[y]
        //        tentative_is_better := true
        //    if tentative_is_better = true
        //        came_from[y] := x
        //        g_score[y] := tentative_g_score
        //        f_score[y] := g_score[y] + h_score[y] % Estimated total distance from start to goal through y.

		break;
	}

	glEnd();

    return openlist.empty();

}
*/

//=================================================================

struct ShapeNode
{
	b2Vec2 pos;
	b2Vec2 normal1;
	b2Vec2 normal2;
	b2Shape *shape;
	unsigned int vertex;
	ShapeNode(const b2Vec2 &pos, const b2Vec2 &normal1, const b2Vec2 &normal2, b2Shape *shape = NULL, unsigned int vertex = ~0U)
		: pos(pos), normal1(normal1), normal2(normal2), shape(shape), vertex(vertex)
	{
	}
};

struct GroupInfo
{
	std::vector<ShapeNode> nodes;
	std::vector<ShapeNode> hull;

	GroupInfo()
	{
	}
};

static std::map<b2Shape *, unsigned int> shapegroup;	// map shape to group index
static std::vector<GroupInfo> groups;		// shape node arrays associated with a group index

struct PathNode
{
	b2Vec2 pos;

	float g_score;
	float h_score;
	float f_score;
	unsigned int come_from;

	unsigned int groupindex;
	unsigned int vertexindex;

	PathNode(const b2Vec2 &pos, float f_score = FLT_MAX, float g_score = FLT_MAX, float h_score = 0.0f, unsigned int groupindex = ~0U, unsigned int vertexindex = ~0U)
		: pos(pos), f_score(f_score), g_score(g_score), h_score(h_score), come_from(~0U), groupindex(groupindex), vertexindex(vertexindex)
	{
	}
};

typedef std::pair<unsigned int, unsigned int> ProbeEntry;

// static state
static unsigned int statichash;
static std::vector<PathNode> node;					// array of potential path nodes
static std::map<unsigned int, unsigned int> index;	// map vertex hash to node index
static std::deque<ProbeEntry> probe;				// priority queue of segment probes
static std::map<unsigned int, bool> visited;		// visited mark
static int steps;

float GetProbeScore(const ProbeEntry &p)
{
	return node[p.first].g_score + b2Distance(node[p.first].pos, node[p.second].pos) + node[p.second].h_score;
}

struct ProbeEntryCompare
{
	bool operator()(const ProbeEntry &e1, const ProbeEntry &e2)
	{
#if 1
		float s1 = GetProbeScore(e1);
		float s2 = GetProbeScore(e2);
		return (s1 > s2) || (s1 == s2 && &e1 > &e2);
#else
		return 
			(node[e1.first].f_score > node[e2.first].f_score) ||
			(node[e1.first].f_score == node[e2.first].f_score && node[e1.second].f_score > node[e2.second].f_score);
#endif
	}
};

struct HullAngleCompare
{
	b2Vec2 pivot;

	HullAngleCompare(const b2Vec2 &pivot)
		: pivot(pivot)
	{
	}

	bool operator()(const ShapeNode &e1, const ShapeNode &e2)
	{
		float a1 = -atan2f(e1.pos.x - pivot.x, e1.pos.y - pivot.y);
		float a2 = -atan2f(e2.pos.x - pivot.x, e2.pos.y - pivot.y);
		return (a1 > a2) || (a1 == a2 && b2DistanceSquared(e1.pos, pivot) > b2DistanceSquared(e2.pos, pivot));
	}
};

static void GetHull(unsigned int groupindex)
{
/*
	Find pivot P;
	Sort Points by angle (with points with equal angle further sorted by distance from P);
	 
	# Points[1] is the pivot
	Stack.push(Points[1]);
	Stack.push(Points[2]);
	FOR i = 3 TO Points.length
			WHILE Stack.length >= 2 and Cross_product(Stack.second, Stack.top, Points[i]) <= 0
					Stack.pop;
			ENDWHILE
			Stack.push(Points[i]);
	NEXT i
	 
	FUNCTION Cross_product(p1, p2, p3)
			RETURN (p2.x - p1.x)*(p3.y - p1.y) - (p3.x - p1.x)*(p2.y - p1.y);
	ENDFUNCTION
*/

	std::vector<ShapeNode> &groupnodes = groups[groupindex].nodes;
	std::vector<ShapeNode> &grouphull = groups[groupindex].hull;

	// clear the hull
	grouphull.clear();

	// find pivot element
	unsigned int pivotindex = 0;
	for (unsigned int i = 1; i < groupnodes.size(); ++i)
	{
		if ((groupnodes[i].pos.y < groupnodes[pivotindex].pos.y) ||
			(groupnodes[i].pos.y == groupnodes[pivotindex].pos.y && groupnodes[i].pos.x < groupnodes[pivotindex].pos.x))
			pivotindex = i;
	}

	// angle comparison
	HullAngleCompare anglecompare(groupnodes[pivotindex].pos);

	// copy group nodes
	std::deque<ShapeNode> source;
	for (unsigned int i = 0; i < groupnodes.size(); ++i)
		source.push_back(groupnodes[i]);

	// swap pivot to start
	source[0] = groupnodes[pivotindex];
	source[pivotindex] = groupnodes[0];

	// add the pivot
	grouphull.push_back(source.front());
	source.pop_front();

	// for each source node...
	while (!source.empty())
	{
		// if the previous node is not on the hull...
		std::make_heap(source.begin(), source.end(), anglecompare);
		while (grouphull.size() >= 2 
			? b2Cross(grouphull[grouphull.size()-1].pos - grouphull[grouphull.size()-2].pos, source.front().pos - grouphull[grouphull.size()-2].pos) <= 0
			: b2DistanceSquared(grouphull[grouphull.size()-1].pos, source.front().pos) == 0.0f)
		{
			// remove the node
			grouphull.pop_back();
		}

		// add the new node
		grouphull.push_back(source.front());
		source.pop_front();
	}

	// generate hull normals
	for (unsigned int i = 0; i < grouphull.size(); ++i)
	{
		grouphull[i].normal1 = b2Cross(grouphull[i < grouphull.size() - 1 ? i + 1 : 0].pos - grouphull[i].pos, 1.0f);
		grouphull[i].normal1.Normalize();
		grouphull[i].normal2 = b2Cross(grouphull[i].pos - grouphull[i > 0 ? i - 1 : grouphull.size() - 1].pos, 1.0f);
		grouphull[i].normal2.Normalize();
	}
}

static unsigned int GetGroup(b2Shape *shape, unsigned int aCategoryBits, unsigned int aMaskBits, float radius)
{
	// check if the shape is already associated with a group
	std::map<b2Shape *, unsigned int>::iterator itor = shapegroup.find(shape);

	// if so, just return the index
	if (itor != shapegroup.end())
		return itor->second;

	// BUILD SHAPE GROUP

	// get a new group
	groups.push_back(GroupInfo());
	unsigned int groupindex = groups.size() - 1;
	std::vector<ShapeNode> &groupnodes = groups[groupindex].nodes;

	// probe the hit shape
	std::deque<b2Shape *> shapequeue;
	shapequeue.push_back(shape);
	
	while (!shapequeue.empty())
	{
		// get the next shape
		b2Shape *shape1 = shapequeue.front();
		shapequeue.pop_front();

		// mark as visited
		unsigned int hash;
		hash = Hash(&shape1, sizeof(shape1));
		visited[hash] = true;

		// add the shape to the group
		shapegroup[shape1] = groupindex;

		// get shape local vertices and normals
		int vertexcount = static_cast<b2PolygonShape*>(shape1)->GetVertexCount();
		const b2Vec2 *vertexlocal = static_cast<b2PolygonShape*>(shape1)->GetVertices();
		const b2Vec2 *normallocal = static_cast<b2PolygonShape*>(shape1)->GetNormals();

		// get padded vertices in world space
		b2Body *body1 = shape1->GetBody();
		b2Vec2 *vertexworld = static_cast<b2Vec2 *>(_alloca(vertexcount*sizeof(b2Vec2)));
		b2Vec2 *normalworld = static_cast<b2Vec2 *>(_alloca(vertexcount*sizeof(b2Vec2)));
		for (int i = 0; i < vertexcount; ++i)
		{
			vertexworld[i] = body1->GetWorldPoint(vertexlocal[i] + radius * (normallocal[i] + normallocal[i > 0 ? i - 1 : vertexcount - 1]));
			normalworld[i] = body1->GetWorldVector(normallocal[i]);
		}

		// point inside?
		bool inside[b2_maxPolygonVertices] = { false };

		// get the collision world
		b2World *world = Collidable::GetWorld();

		// get nearby shapes
		b2AABB aabb;
		shape1->ComputeAABB(&aabb, body1->GetXForm());
		aabb.lowerBound.x -= radius + radius;
		aabb.lowerBound.y -= radius + radius;
		aabb.upperBound.x += radius + radius;
		aabb.upperBound.y += radius + radius;
		b2Shape* shapes[b2_maxProxies];
		int32 count = world->Query(aabb, shapes, b2_maxProxies);

		// for each shape...
		for (int32 i = 0; i < count; ++i)
		{
			// get the shape
			b2Shape* shape2 = shapes[i];

			// skip self
			if (shape1 == shape2)
				continue;

			// skip unhittable shapes
			if (shape2->IsSensor())
				continue;
			if ((shape2->GetFilterData().maskBits & aCategoryBits) == 0)
				continue;
			if ((shape2->GetFilterData().categoryBits & aMaskBits) == 0)
				continue;

			// get the shape body
			b2Body *body2 = shape2->GetBody();

			// check for points inside
			for (int32 j = 0; j < vertexcount; ++j)
			{
				inside[j] = inside[j] || shape2->TestPoint(body2->GetXForm(), vertexworld[j], radius);
			}

			// skip visited shapes
			unsigned int hash;
			hash = Hash(&shape2, sizeof(shape2));
			if (visited.find(hash) != visited.end())
				continue;
			visited[hash] = true;

			// if the shapes overlap...
			b2Vec2 p1, p2;
			if (b2Distance(&p1, &p2, shape1, body1->GetXForm(), shape2, body2->GetXForm()) < radius + radius)
			{
				// probe the shape
				shapequeue.push_back(shape2);
			}
		}

		// process vertices
		for (int i = 0; i < vertexcount; ++i)
		{
			// skip vertices inside other shapes
			if (inside[i])
				continue;

			// add the vertex to the group
			groupnodes.push_back(ShapeNode(vertexworld[i], normalworld[i], normalworld[i > 0 ? i - 1 : vertexcount - 1], shape1, i));
		}
	}

	GetHull(groupindex);

	return groupindex;
}

void AddProbe(unsigned int from, unsigned int to)
{
	// mark the link as visited
	unsigned int key = (from << 16) | to;
	if (visited.find(key) == visited.end())
	{
		probe.push_back(ProbeEntry(from, to));
		visited[key] = true;
	}
}

bool Pathing(const b2Vec2 &start, const b2Vec2 &goal, float radius)
{
	const unsigned int aCategoryBits = 0xFFFF;
	const unsigned int aMaskBits = 0x0001;

	// get signature
	unsigned int statehash;
	statehash = Hash(&radius, sizeof(radius));
	statehash = Hash(&start, sizeof(start), statehash);
	statehash = Hash(&goal, sizeof(goal), statehash);

	// if the signature changed...
	if (statichash != statehash)
	{
		// update signature
		statichash = statehash;

		// clear static state
		node.clear();
		index.clear();
		probe.clear();
		visited.clear();
		steps = 0;

		DebugPrint("Pathing (%f,%f)->(%f,%f)\n", start.x, start.y, goal.x, goal.y);

		// add start node
		node.push_back(PathNode(start, b2Distance(start, goal), 0, b2Distance(start, goal)));

		// add goal node
		node.push_back(PathNode(goal, FLT_MAX, FLT_MAX, 0));

		// generate a probe towards the goal
		AddProbe(0, 1);
	}

	glBegin(GL_LINES);

	// goal node
	PathNode &goalnode = node[1];

	// compare node values
	ProbeEntryCompare probeentrycompare;

	// while there are probes queued
	while(!probe.empty())
	{
		// get the probe with the best (potential) f_score
		std::make_heap(probe.begin(), probe.end(), probeentrycompare);
		ProbeEntry entry(probe.front());
		probe.pop_front();

		// get the destination node
		PathNode &secondnode = node[entry.second];

		// skip if the goal has a better path
		// (an intervening probe reached it first)
		if (secondnode.f_score > goalnode.f_score)
			continue;

		DebugPrint("%d: %d->%d %f\n", steps, entry.first, entry.second, secondnode.f_score);
		++steps;

		// get the source node
		PathNode &firstnode = node[entry.first];

		// get the segment
		b2Segment segment;
		segment.p1 = firstnode.pos;
		segment.p2 = secondnode.pos;

		// impact point
		float lambda = 1.0f;
		b2Vec2 normal(0.0f, 0.0f);
		b2Shape *shape = NULL;

		// get segment intersection
		// HACK: check collision against environment only
		if (!Collidable::TestSegment(segment, radius - 0.0625f, 0, aCategoryBits, aMaskBits, lambda, normal, shape))
		{
			// draw segment
			glColor4f(0.0f, 1.0f, 0.0f, 1.0f);
			glVertex2f(segment.p1.x, segment.p1.y);
			glVertex2f(segment.p2.x, segment.p2.y);

			// if the segment improves the node's score...
			float tentative_g_score = firstnode.g_score + b2Distance(segment.p1, segment.p2);
			if (secondnode.g_score > tentative_g_score)
			{
				// update the node
				secondnode.g_score = tentative_g_score;
				secondnode.f_score = secondnode.g_score + secondnode.h_score;
				secondnode.come_from = entry.first;

				// if the first node has a predecessor...
				unsigned int from = firstnode.come_from;
				if (from != ~0U)
				{
					// try to snap the link
					// (it may have already been visited)
					AddProbe(from, entry.second);
				}

				// if the second node is not the goal...
				if (entry.second != 1)
				{
					// generate a segment towards the goal
					AddProbe(entry.second, 1);
				}
			}
		}
		else
		{
			// get the shape group
			unsigned int groupindex = GetGroup(shape, aCategoryBits, aMaskBits, radius);

//#define SKIP_VISITED_GROUPS
#ifdef SKIP_VISITED_GROUPS
			// mark as visited
			unsigned int hash;
			hash = Hash(&entry.first, sizeof(entry.first));
			hash = Hash(&groupindex, sizeof(groupindex), hash);
			++hash;
			if (visited.find(hash) != visited.end())
				continue;
			visited[hash] = true;
#endif

			// draw segment
			b2Vec2 intersect(Lerp(segment.p1, segment.p2, lambda));
			glColor4f(1.0f, 0.0f, 0.0f, 1.0f);
			glVertex2f(segment.p1.x, segment.p1.y);
			glVertex2f(intersect.x, intersect.y);
			glColor4f(0.0f, 0.0f, 1.0f, 1.0f);
			glVertex2f(intersect.x, intersect.y);
			glVertex2f(segment.p2.x, segment.p2.y);

			// get group info
			GroupInfo &groupinfo = groups[groupindex];

			// get group shape nodes
			std::vector<ShapeNode> &groupnodes = groupinfo.nodes;
			std::vector<ShapeNode> &grouphull = groupinfo.hull;

#ifdef SKIP_OCCLUDE_START_SHAPE
			// if the segment starts on a shape...
			ShapeNode *sourcenode = (firstnode.groupindex != ~0U)
				? &groups[firstnode.groupindex][firstnode.vertexindex]
				: NULL;
#endif

			// check if both ends are outside the group's convex hull
			bool outside1 = false, outside2 = false;
			for (unsigned int i = 0; i < grouphull.size(); ++i)
			{
				outside1 = outside1 || b2Dot(segment.p1 - grouphull[i].pos, grouphull[i].normal1) > -FLT_EPSILON;
				outside2 = outside2 || b2Dot(segment.p2 - grouphull[i].pos, grouphull[i].normal1) > -FLT_EPSILON;
			}
			bool outside = outside1 && outside2;

			// vertices to use
			std::vector<ShapeNode> &groupverts = outside ? grouphull : groupnodes;

			// for each group node...
			for (unsigned int i = 0; i < groupverts.size(); ++i)
			{
				// get the shape node
				ShapeNode &shapenode = groupverts[i];

//#define DEBUG_DRAW_GROUP_VERTICES
#ifdef DEBUG_DRAW_GROUP_VERTICES
				glColor4f(1.0f, 1.0f, 1.0f, 0.25f);
				glVertex2f(shapenode.pos.x, shapenode.pos.y);
				glVertex2f(shapenode.pos.x + 3*shapenode.normal1.x, shapenode.pos.y + 3*shapenode.normal1.y);
				glVertex2f(shapenode.pos.x, shapenode.pos.y);
				glVertex2f(shapenode.pos.x + 3*shapenode.normal2.x, shapenode.pos.y + 3*shapenode.normal2.y);
#endif

				// direction to the vertex
				const b2Vec2 dir = shapenode.pos - segment.p1;

#ifdef SKIP_VISITED_GROUPS
				// skip if occluded by the shape
				if (b2Dot(dir, shapenode.normal1) > 0.0f &&
					b2Dot(dir, shapenode.normal2) > 0.0f)
#else
				// skip "non-silhouette" vertices
				if (b2Dot(dir, shapenode.normal1) * b2Dot(dir, shapenode.normal2) > 0.0f)
#endif
				{
#ifdef DEBUG_DRAW_GROUP_VERTICES
					glColor4f(1.0f, 0.0f, 1.0f, 0.25f);
					glVertex2f(segment.p1.x, segment.p1.y);
					glVertex2f(shapenode.pos.x, shapenode.pos.y);
#endif
					continue;
				}

#ifdef DEBUG_DRAW_GROUP_VERTICES
				glColor4f(0.0f, 1.0f, 1.0f, 0.5f);
				glVertex2f(segment.p1.x, segment.p1.y);
				glVertex2f(shapenode.pos.x, shapenode.pos.y);
#endif

#ifdef SKIP_OCCLUDE_START_SHAPE
				// skip if occluded by the segment's owning shape
				if (sourcenode && 
					b2Dot(dir, sourcenode->normal1) < -FLT_EPSILON && 
					b2Dot(dir, sourcenode->normal2) < -FLT_EPSILON)
					continue;
#endif

				// get target score
				float g_score = firstnode.g_score + b2Distance(segment.p1, shapenode.pos);
				float h_score = b2Distance(shapenode.pos, goal);
				float f_score = g_score + h_score;

				// skip if the goal has a better path
				if (goalnode.f_score < f_score)
					continue;

				// if the vertex does not correspond to a path node...
				int vertexhash = Hash(&shapenode.pos, sizeof(shapenode.pos));
				unsigned int vertexindex;
				std::map<unsigned int, unsigned int>::iterator indexitor = index.find(vertexhash);
				if (indexitor == index.end())
				{
					// add a new path node
					node.push_back(PathNode(shapenode.pos, f_score, FLT_MAX, h_score, groupindex, i));
					vertexindex = node.size() - 1;
					index[vertexhash] = vertexindex;

					// add a probe
					AddProbe(entry.first, vertexindex);
				}
				else
				{
					// use the node index
					vertexindex = indexitor->second;

					// if the probe could improve the score...
					if (node[vertexindex].g_score > g_score)
					{
						// add a probe
						AddProbe(entry.first, vertexindex);
					}
				}

			}
		}

		// only permit one per frame
		break;
	}

	// for each path segment...
	for (unsigned int i = 1; node[i].come_from != ~0U; i = node[i].come_from)
	{
		const b2Vec2 &p1 = node[i].pos;
		const b2Vec2 &p2 = node[node[i].come_from].pos;

		// draw segment
		glColor4f(0.0f, 1.0f, 0.0f, 1.0f);
		glVertex2f(p1.x, p1.y);
		glVertex2f(p2.x, p2.y);
	}

	glEnd();

	return probe.empty();
}

static GLuint grid_handle = 0;
Color4 cell_color[] =
{
	{ 0.0f, 0.5f, 1.0f, 0.5f },
	{ 1.0f, 0.0f, 0.0f, 1.0f },
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

	// create a probe shape
	b2PolygonDef probedef;
	probedef.SetAsBox(aCellSize * 0.5f, aCellSize * 0.5f, b2Vec2(aCellSize * 0.5f, aCellSize * 0.5f), 0.0f);
	probedef.isSensor = true;
	b2Shape *probe = world->GetGroundBody()->CreateShape(&probedef);

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
					// get shapes intersecting the cell
					b2AABB cell_aabb;
					cell_aabb.lowerBound.x = zone_aabb.lowerBound.x + (col) * aCellSize;
					cell_aabb.lowerBound.y = zone_aabb.lowerBound.y + (row) * aCellSize;
					cell_aabb.upperBound.x = zone_aabb.lowerBound.x + (col + 1) * aCellSize;
					cell_aabb.upperBound.y = zone_aabb.lowerBound.y + (row + 1) * aCellSize;
					b2Shape* shapes[b2_maxProxies];
					int count = world->Query(cell_aabb, shapes, b2_maxProxies);

					// probe transform
					b2XForm probe_xform(cell_aabb.lowerBound, b2Mat22(1, 0, 0, 1));

					static const unsigned int aCategoryBits = 1 << 0;
					static const unsigned int aMaskBits = 0xFFFF;

					bool blocked = false;
					for (int i = 0; i < count; ++i)
					{
						// get the shape
						b2Shape *shape = shapes[i];

						// skip unhittable shapes
						if (shape->IsSensor())
							continue;
						if ((shape->GetFilterData().maskBits & aCategoryBits) == 0)
							continue;
						if ((shape->GetFilterData().categoryBits & aMaskBits) == 0)
							continue;

						// get the parent body
						b2Body* body = shape->GetBody();
						if (!body->IsStatic())
							continue;

						// probe intersection
						b2Vec2 x1, x2;
						if (b2Distance(&x1, &x2, probe, probe_xform, shape, body->GetXForm()) <= 0.0f)
						{
							blocked = true;
							break;
						}
					}

					// cell type (0=empty, 1=blocked)
					cell_map[row * cell_side + col] = blocked;
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

	// destroy the probe shape
	world->GetGroundBody()->DestroyShape(probe);

	glEndList();
}

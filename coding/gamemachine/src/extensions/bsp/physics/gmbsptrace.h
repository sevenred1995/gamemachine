﻿#ifndef __BSPTRACE_H__
#define __BSPTRACE_H__
#include <gmcommon.h>
#include <gmbsp.h>
#include "gmbspphysicsstructs.h"
BEGIN_NS

#define MAX_TOUCHED_ENTITY_NUM 1024

GM_ALIGNED_STRUCT_FROM(BSPTracePlane, BSPPlane)
{
	GMbyte planeType = 0;
	GMint signbits = 0;

	BSPTracePlane& operator =(const BSPTracePlane& p)
	{
		BSPPlane::operator=(p);

		this->planeType = p.planeType;
		this->signbits = p.signbits;
		return *this;
	}

	BSPTracePlane& operator =(const BSPPlane& p)
	{
		BSPPlane::operator=(p);
		return *this;
	}
};

GM_ALIGNED_STRUCT(BSPTraceResult)
{
	bool allsolid = false; // if true, plane is not valid
	bool startsolid = false; // if true, the initial point was in a solid area
	GMfloat fraction = 0; // time completed, 1.0 = didn't hit anything
	glm::vec3 endpos{ 0 }; // final position
	BSPTracePlane plane; // surface normal at impact, transformed to world space
	GMint surfaceFlags = 0; // surface hit
	GMint contents = 0; // contents on other side of surface hit

	GMint entityNum = 0;
	GMint entities[MAX_TOUCHED_ENTITY_NUM]{ 0 };
};

GM_ALIGNED_STRUCT(BSPSphere)
{
	glm::vec3 offset{ 0 };
	GMfloat radius = false;
	GMfloat halfheight = false;
	bool use = false;
};

class GMBSPPhysicsWorld;
class GMEntityObject;
GM_PRIVATE_OBJECT(GMBSPTrace)
{
	BSPData* bsp;
	Map<GMint, Set<GMBSPEntity*> >* entities;
	Map<GMBSPEntity*, GMEntityObject*>* entityObjects;
	GMBSPPhysicsWorld* world;
	GMint checkcount = 0;
};

struct GMBSP_Physics_Brush;
struct GMBSP_Physics_Patch;
struct GMBSPPatchCollide;
struct GMBSPTraceWork;
class GMBSPTrace
{
	DECLARE_PRIVATE(GMBSPTrace)

public:
	void initTrace(BSPData* bsp, Map<GMint, Set<GMBSPEntity*> >* entities, Map<GMBSPEntity*, GMEntityObject*>* entityObjects, GMBSPPhysicsWorld* world);
	void trace(const glm::vec3& start, const glm::vec3& end, const glm::vec3& origin, const glm::vec3& min, const glm::vec3& max, REF BSPTraceResult& trace);
	void traceThroughTree(GMBSPTraceWork& tw, GMint num, GMfloat p1f, GMfloat p2f, const glm::vec3& p1, const glm::vec3& p2);
	void traceThroughLeaf(GMBSPTraceWork& tw, GMBSPLeaf* leaf);
	void traceThroughBrush(GMBSPTraceWork& tw, GMBSP_Physics_Brush* brush);
	void traceThroughPatch(GMBSPTraceWork& tw, GMBSP_Physics_Patch* patch);
	void traceEntityThroughLeaf(GMBSPTraceWork& tw, std::set<GMBSPEntity*>& entities);
	void traceThroughPatchCollide(GMBSPTraceWork& tw, GMBSPPatchCollide* pc);
	void tracePointThroughPatchCollide(GMBSPTraceWork& tw, const GMBSPPatchCollide *pc);
	GMint checkFacetPlane(const glm::vec4& plane, const glm::vec3& start, const glm::vec3& end, GMfloat *enterFrac, GMfloat *leaveFrac, GMint *hit);
	bool boundsIntersect(const glm::vec3& mins, const glm::vec3& maxs, const glm::vec3& mins2, const glm::vec3& maxs2);
};

END_NS
#endif
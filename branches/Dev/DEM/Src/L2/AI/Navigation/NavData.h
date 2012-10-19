#pragma once
#ifndef __DEM_L2_AI_NAV_DATA_H__
#define __DEM_L2_AI_NAV_DATA_H__

#include <Data/StringID.h>
#include <util/nfixedarray.h>
#include <util/ndictionary.h>
#include <DetourNavMesh.h>

// AI level is an abstract space (i.e. some of location views, like GfxLevel & PhysicsLevel),
// that contains stimuli, AI hints and other AI-related world info. Also AILevel serves as
// a navigation manager.

static const int MAX_NAV_PATH = 256;
static const int MAX_ITERS_PER_UPDATE = 100;
static const int MAX_PATHQUEUE_NODES = 4096;
static const int MAX_COMMON_NODES = 512;

class dtNavMeshQuery;

namespace Data
{
	class CStream;
}

namespace AI
{
typedef nFixedArray<dtPolyRef> CNavRegion;

class CNavData
{
public:

	float							AgentRadius;
	float							AgentHeight;

	dtNavMesh*						pNavMesh;
	dtNavMeshQuery*					pNavMeshQuery[DEM_THREAD_COUNT]; // [0] is sync, main query

	nDictionary<CStrID, CNavRegion> Regions;

	CNavData(): pNavMesh(NULL) { memset(pNavMeshQuery, 0, sizeof(pNavMeshQuery)); }

	bool LoadFromStream(Data::CStream& Stream);
	void Clear();
};

}

#endif
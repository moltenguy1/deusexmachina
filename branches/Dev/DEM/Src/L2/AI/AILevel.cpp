#include "AILevel.h"

#include <AI/AIServer.h>
#include <AI/Perception/Sensor.h>
#include <Data/Streams/FileStream.h>
#include <Events/EventManager.h>
#include <DetourCommon.h>
#include <DetourNavMeshQuery.h>
#include <DetourPathCorridor.h>

namespace AI
{

CAILevel::~CAILevel()
{
	UnloadNavMesh();
}
//---------------------------------------------------------------------

bool CAILevel::Init(const bbox3& LevelBox, uchar QuadTreeDepth)
{
	Box = LevelBox;
	vector3 Center = LevelBox.center();
	vector3 Size = LevelBox.size();
	StimulusQT.Build(Center.x, Center.z, Size.x, Size.z, QuadTreeDepth);
	OK;
}
//---------------------------------------------------------------------

bool CAILevel::LoadNavMesh(const nString& FileName)
{
	Data::CFileStream File;
	if (!File.Open(FileName, Data::SAM_READ)) FAIL;

	if (File.Get<int>() != '_NM_') FAIL;
	int Version = File.Get<int>();

	int NMCount = File.Get<int>();
	for (int NMIdx = 0; NMIdx < NMCount; ++NMIdx)
	{
		float Radius = File.Get<float>();
		CNavData& New = NavData.Add(Radius);
		New.AgentRadius = Radius;
		New.AgentHeight = File.Get<float>();
		if (!New.LoadFromStream(File))
		{
			UnloadNavMesh();
			FAIL;
		}
	}

	EventMgr->FireEvent(CStrID("OnNavMeshDataChanged"));
	OK;
}
//---------------------------------------------------------------------

void CAILevel::UnloadNavMesh()
{
	for (int i = 0; i < NavData.Size(); ++i)
	{
		CNavData& Data = NavData.ValueAtIndex(i);
		Data.Clear();
	}
	NavData.Clear();
	EventMgr->FireEvent(CStrID("OnNavMeshDataChanged"));
}
//---------------------------------------------------------------------

CStimulusNode* CAILevel::RegisterStimulus(CStimulus* pStimulus)
{
	n_assert(pStimulus && !pStimulus->GetQuadTreeNode());
	return StimulusQT.AddObject(pStimulus);
}
//---------------------------------------------------------------------

void CAILevel::RemoveStimulus(CStimulus* pStimulus)
{
	n_assert(pStimulus && pStimulus->GetQuadTreeNode());
	StimulusQT.RemoveObject(pStimulus);
}
//---------------------------------------------------------------------

void CAILevel::RemoveStimulus(CStimulusNode* pStimulusNode)
{
	n_assert(pStimulusNode && pStimulusNode->Object->GetQuadTreeNode());
	StimulusQT.RemoveObject(pStimulusNode);
}
//---------------------------------------------------------------------

void CAILevel::QTNodeUpdateActorsSense(CStimulusQT::CNode* pNode, CActor* pActor, CSensor* pSensor, EClipStatus ClipStatus)
{
	if (!pNode->GetTotalObjCount()) return;

	if (ClipStatus == InvalidClipStatus || ClipStatus == Clipped)
	{
		bbox3 BBox;
		pNode->GetBounds(BBox);
		BBox.vmin.y = Box.vmin.y;
		BBox.vmax.y = Box.vmax.y;
		ClipStatus = pSensor->GetBoxClipStatus(pActor, BBox);
	}

	if (ClipStatus == Outside) return;

	for (int i = 0; i < pNode->Data.GetListCount(); ++i)
		if (pSensor->AcceptsStimulusType(*pNode->Data.GetKeyAt(i)))
		{
			CStimulusListSet::CElement* pCurr = pNode->Data.GetHeadAt(i);
			for (; pCurr; pCurr = pCurr->GetSucc())
				pSensor->SenseStimulus(pActor, pCurr->Object);
		}

	if (pNode->HasChildren())
		for (DWORD i = 0; i < 4; i++)
			QTNodeUpdateActorsSense(pNode->GetChild(i), pActor, pSensor, ClipStatus);
}
//---------------------------------------------------------------------

CNavData* CAILevel::GetNavData(float ActorRadius)
{
	// NavData is assumed to be sorted by key (agent radius) in ascending order
	for (int i = 0; i < NavData.Size(); ++i)
		if (ActorRadius <= NavData.KeyAtIndex(i))
			return &NavData.ValueAtIndex(i);

	return NULL;
}
//---------------------------------------------------------------------

bool CAILevel::GetAsyncNavQuery(float ActorRadius, dtNavMeshQuery*& pOutQuery, CPathRequestQueue*& pOutQueue)
{
	CNavData* pNav = GetNavData(ActorRadius);
	if (!pNav) FAIL;

	//!!!Select least loaded thread...
	DWORD ThreadID = 0;

	pOutQuery = pNav->pNavMeshQuery[ThreadID];
	pOutQueue = AISrv->GetPathQueue(ThreadID);
	OK;
}
//---------------------------------------------------------------------

} //namespace AI
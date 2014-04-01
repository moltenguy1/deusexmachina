#pragma once
#ifndef __DEM_L3_DLG_CONTEXT_H__
#define __DEM_L3_DLG_CONTEXT_H__

#include <Dlg/DlgGraph.h>

// Dialogue context is an instance that stores execution state associated with a dialogue graph

namespace Story
{

enum EDlgState
{
	DlgState_Requested,	// Requested, not accepted by target
	DlgState_InNode,	// Accepted, current node must be processed
	DlgState_Waiting,	// Current node was processed, wait for a time, UI or other response
	DlgState_InLink,	// Response received, follow selected link
	DlgState_Finished,	// Exited node with no valid links or with a link to NULL
	DlgState_Aborted	// Rejected or failed to execute script (or aborted by user?)
};

class CDlgContext //???struct?
{
public:

	PDlgGraph	Dlg;
	EDlgState	State;

	CStrID		Initiator;
	CStrID		Target;
	CStrID		DlgOwner;
	CStrID		PlrSpeaker;

	CDlgNode*	pCurrNode;
	//float		NodeEnterTime;
	int			LinkIdx;
	CArray<int>	ValidLinkIndices;	// For nodes with delayed link selection, like answer nodes

	CDlgContext(): pCurrNode(NULL) {}

	void HandleNode();
	void HandleLink();
	void SelectValidLink(int Idx);
};

}

#endif
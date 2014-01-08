#pragma once
#ifndef __DEM_L2_AI_ACTION_GOTO_SMART_OBJ_H__
#define __DEM_L2_AI_ACTION_GOTO_SMART_OBJ_H__

#include <AI/Movement/Actions/ActionGoto.h>
#include <Data/StringID.h>

// Goto action that sets destination from smart object, calculating destination.

namespace Prop
{
	class CPropSmartObject;
}

namespace AI
{

class CActionGotoSmartObj: public CActionGoto
{
	__DeclareClass(CActionGotoSmartObj);

private:

	CStrID	TargetID;
	CStrID	ActionID;

	bool UpdateDestination(CActor* pActor, Prop::CPropSmartObject* pSO);

public:

	void				Init(CStrID Target, CStrID Action) { TargetID = Target; ActionID = Action; }
	virtual bool		Activate(CActor* pActor);
	virtual DWORD	Update(CActor* pActor);

	virtual void		GetDebugString(CString& Out) const { Out.Format("%s(%s, %s)", GetClassName().CStr(), TargetID.CStr(), ActionID.CStr()); }
};

typedef Ptr<CActionGotoSmartObj> PActionGotoSmartObj;

}

#endif
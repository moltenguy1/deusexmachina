#pragma once
#ifndef __DEM_L3_PROP_TALKING_H__
#define __DEM_L3_PROP_TALKING_H__

#include <Game/Property.h>
#include <StdDEM.h>

// Adds ability to talk to the actor. Actor can speak in a conversation or spell
// background phrases, that appear above its head/top.

namespace Story
{
	typedef Ptr<class CDialogue> PDialogue;
}

namespace Prop
{
using namespace Events;
using namespace Story;

class CPropTalking: public Game::CProperty
{
	__DeclareClass(CPropTalking);
	__DeclarePropertyStorage;

protected:

	//??????!!!!!or just CStrID dlg name & resolve in DlgMgr
	PDialogue Dialogue;

	//broken dialogue state (to restart or take into account)

	DECLARE_EVENT_HANDLER(ExposeSI, ExposeSI);
	DECLARE_EVENT_HANDLER(Talk, OnTalk);

public:

	virtual void	Activate();
	virtual void	Deactivate();
	virtual void	SayPhrase(CStrID PhraseID);

	CDialogue*		GetDialogue() { return Dialogue.GetUnsafe(); }
};

}

#endif
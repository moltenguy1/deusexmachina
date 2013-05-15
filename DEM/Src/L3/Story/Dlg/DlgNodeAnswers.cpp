#include "DlgNodeAnswers.h"

#include "DialogueManager.h"
#include "DlgLink.h"
#include "DlgNodePhrase.h"
#include <Events/EventManager.h>

namespace Story
{
__ImplementClassNoFactory(Story::CDlgNodeAnswers, Story::CDlgNode);
__ImplementClass(Story::CDlgNodeAnswers);

void CDlgNodeAnswers::OnEnter(CActiveDlg& Dlg)
{
	DlgMgr->SayPhrase(SpeakerEntity, Phrase, Dlg);
}
//---------------------------------------------------------------------

CDlgNode* CDlgNodeAnswers::Trigger(CActiveDlg& Dlg)
{
#ifdef _DEBUG
	n_assert2(DlgMgr->IsDialogueForeground(Dlg) && Dlg.PlrSpeaker.IsValid(),
		"Answer node can be used only in a foreground dialogue with player");
#endif

	if (Dlg.IsCheckingConditions)
	{
		while (Dlg.LinkIdx < Links.GetCount())
		{
			EExecStatus Status = Links[Dlg.LinkIdx]->Validate(Dlg);
			if (Status == Success)
			{
				n_assert2(Links[Dlg.LinkIdx]->pTargetNode->IsA(CDlgNodePhrase::RTTI), "Answer dlg node should contain ONLY phrase nodes!");
				
				Dlg.ValidLinkIndices.Append(Dlg.LinkIdx);

				//???send link index too?
				PParams P = n_new(CParams);
				P->Set(CStrID("Phrase"), (PVOID)((CDlgNodePhrase*)Links[Dlg.LinkIdx]->pTargetNode)->Phrase.CStr());
				EventMgr->FireEvent(CStrID("OnDlgAnswerVariantAdded"), P);
			}
			else if (Status == Running) return this;
			Dlg.LinkIdx++;
		}

		EventMgr->FireEvent(CStrID((Dlg.ValidLinkIndices.GetCount() > 0) ? "OnDlgAnswersAvailable" : "OnDlgEndAvailable"));
		Dlg.IsCheckingConditions = false;
	}

	if (!Dlg.Continued) return this;

	return (Dlg.LinkIdx == Links.GetCount()) ? NULL : Links[Dlg.LinkIdx]->DoTransition(Dlg);
}
//---------------------------------------------------------------------

} //namespace AI
#include "NodeControllerComposite.h"

namespace Scene
{
__ImplementClassNoFactory(Scene::CNodeControllerComposite, CNodeController);

bool CNodeControllerComposite::OnAttachToNode(Scene::CSceneNode* pSceneNode)
{
	//if (!CNodeController::OnAttachToNode(pSceneNode)) FAIL;
	pNode = pSceneNode;
	for (int i = 0; i < Sources.GetCount(); ++i)
		Sources[i].Ctlr->OnAttachToNode(pSceneNode);
	OK;
}
//---------------------------------------------------------------------

void CNodeControllerComposite::OnDetachFromNode()
{
	for (int i = 0; i < Sources.GetCount(); ++i)
		Sources[i].Ctlr->OnDetachFromNode();
	//CNodeController::OnDetachFromNode();
	pNode = NULL;
}
//---------------------------------------------------------------------

bool CNodeControllerComposite::AddSource(Scene::CNodeController& Ctlr, DWORD Priority, float Weight)
{
#ifdef _DEBUG
	for (int i = 0; i < Sources.GetCount(); ++i)
		if (Sources[i].Ctlr.GetUnsafe() == &Ctlr) FAIL;
#endif

	if (Sources.GetCount())
	{
		if (!NeedToUpdateLocalSpace() && Ctlr.NeedToUpdateLocalSpace())
			Flags.Set(UpdateLocalSpace);
	}
	else
	{
		Flags.SetTo(LocalSpace, Ctlr.IsLocalSpace());
		Flags.SetTo(UpdateLocalSpace, Ctlr.NeedToUpdateLocalSpace());
	}

	Channels.Set(Ctlr.GetChannels());

	CSource Src;
	Src.Ctlr = &Ctlr;
	Src.Priority = Priority;
	Src.Weight = Weight;
	Sources.InsertSorted(Src);

	if (pNode) Src.Ctlr->OnAttachToNode(pNode);

	OK;
}
//---------------------------------------------------------------------

bool CNodeControllerComposite::RemoveSource(const Scene::CNodeController& Ctlr)
{
	for (int i = 0; i < Sources.GetCount(); ++i)
	{
		CSource& Src = Sources[i];
		if (Src.Ctlr.GetUnsafe() == &Ctlr)
		{
			if (pNode) Src.Ctlr->OnDetachFromNode();

			Sources.EraseAt(i);

			Channels.ClearAll();
			for (int j = 0; j < Sources.GetCount(); ++j)
				Channels.Set(Sources[j].Ctlr->GetChannels());

			OK;
		}
	}
	FAIL;
}
//---------------------------------------------------------------------

void CNodeControllerComposite::Clear()
{
	Channels.ClearAll();
	Sources.Clear();
}
//---------------------------------------------------------------------

}

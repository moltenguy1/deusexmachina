#include "PropSceneNode.h"

#include <Game/Entity.h>
#include <Game/GameLevel.h>
#include <Scene/SceneServer.h>
#include <Scene/Model.h>

namespace Scene
{
	bool LoadNodesFromSCN(const nString& FileName, PSceneNode RootNode, bool PreloadResources = true);
}

namespace Prop
{
__ImplementClass(Prop::CPropSceneNode, 'PSCN', Game::CProperty);
__ImplementPropertyStorage(CPropSceneNode);

void CPropSceneNode::Activate()
{
	CProperty::Activate();

	nString NodePath;
	GetEntity()->GetAttr<nString>(CStrID("ScenePath"), NodePath);
	nString NodeFile;
	GetEntity()->GetAttr<nString>(CStrID("SceneFile"), NodeFile);

	if (NodePath.IsEmpty() && NodeFile.IsValid())
		NodePath = GetEntity()->GetUID().CStr();
	
	if (NodePath.IsValid())
	{
		//???optimize duplicate search?
		Node = GetEntity()->GetLevel().GetScene()->GetNode(NodePath.CStr(), false);
		ExistingNode = Node.IsValid();
		if (!ExistingNode) Node = GetEntity()->GetLevel().GetScene()->GetNode(NodePath.CStr(), true);
		n_assert(Node.IsValid());

		if (NodeFile.IsValid()) n_assert(Scene::LoadNodesFromSCN("scene:" + NodeFile + ".scn", Node));

		if (ExistingNode)
			GetEntity()->SetAttr<matrix44>(CStrID("Transform"), Node->GetWorldMatrix());
		else Node->SetLocalTransform(GetEntity()->GetAttr<matrix44>(CStrID("Transform"))); //???set local? or set global & then calc local?
	}
}
//---------------------------------------------------------------------

void CPropSceneNode::Deactivate()
{
	if (Node.IsValid() && !ExistingNode)
	{
		Node->RemoveFromParent();
		Node = NULL;
	}
	CProperty::Deactivate();
}
//---------------------------------------------------------------------

//???to node or some scene utils?
void CPropSceneNode::GetAABB(bbox3& OutBox) const
{
	if (!Node.IsValid() || !Node->GetAttrCount()) return;

	OutBox.begin_extend();
	for (DWORD i = 0; i < Node->GetAttrCount(); ++i)
	{
		Scene::CSceneNodeAttr& Attr = *Node->GetAttr(i);
		if (Attr.IsA<Scene::CModel>())
		{
			bbox3 AttrBox;
			((Scene::CModel&)Attr).GetGlobalAABB(AttrBox);
			OutBox.extend(AttrBox);
		}
	}
	OutBox.end_extend();
}
//---------------------------------------------------------------------

}
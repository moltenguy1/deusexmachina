// Loads hierarchy of scene nodes from .scn file.
// Use function declaration instead of header file where you want to call this loader.

#include <Scene/Scene.h> // For owning (pinning) created child nodes, likely to be HACK!
#include <Scene/SceneNode.h>
#include <Scene/Model.h> // For resource preloading only, mb HACK
#include <Data/BinaryReader.h>
#include <Data/Streams/FileStream.h>

namespace Scene
{

bool LoadNodesFromSCN(Data::CStream& In, PSceneNode RootNode, bool PreloadResources = true)
{
	if (!RootNode.isvalid()) FAIL;

	Data::CBinaryReader Reader(In);

	RootNode->SetScale(Reader.Read<vector3>());
	RootNode->SetRotation(Reader.Read<quaternion>());
	RootNode->SetPosition(Reader.Read<vector3>());

	ushort Count;
	Reader.Read(Count);
	for (ushort i = 0; i < Count; ++i)
	{
		ushort DataBlockCount;
		Reader.Read(DataBlockCount);
		--DataBlockCount;

		static const nString StrAttr("Scene::C");
		char ClassName[256];
		n_assert(Reader.ReadString(ClassName, sizeof(ClassName)));

		PSceneNodeAttr Attr = (CSceneNodeAttr*)CoreFct->Create(StrAttr + ClassName);

		//!!!move to Attr->LoadFromStream! some attrs may want to load not by block, but sequentially.
		//may require to read data block count inside, or ignore it for such attrs.
		for (ushort j = 0; j < DataBlockCount; ++j)
			if (!Attr->LoadDataBlock(Reader.Read<int>(), Reader)) FAIL;

		RootNode->AddAttr(*Attr);

		// If false, resources will be loaded at first access (first time node attrs are visible or smth)
		//!!!visibility requires some AABB! 
		if (PreloadResources)
		{
			//!!!HACKY FOR NOW! But mb this is a good way, if NO other attrs have resources
			// Else here must be virtual function call
			if (Attr->IsA(CModel::RTTI))
			{
				CModel* pMesh = (CModel*)Attr.get_unsafe();
				if (!pMesh->AreResourcesValid() && !pMesh->LoadResources()) FAIL;
			}
		}
	}

	Reader.Read(Count);
	for (ushort i = 0; i < Count; ++i)
	{
		char ChildName[256];
		n_assert(Reader.ReadString(ChildName, sizeof(ChildName)));

		//!!!REDUNDANCY! { [ { } ] } problem.
		short SMTH = Reader.Read<short>();

		//!!!revisit it! pinning all child nodes is smth strange!
		//also they won't be unloaded when root node is unloaded!
		PSceneNode Child = RootNode->CreateChild(CStrID(ChildName));
		RootNode->GetScene()->OwnNode(Child);
		if (!LoadNodesFromSCN(In, Child, PreloadResources)) FAIL;
	}

	OK;
}
//---------------------------------------------------------------------

bool LoadNodesFromSCN(const nString& FileName, PSceneNode RootNode, bool PreloadResources = true)
{
	Data::CFileStream File;
	return File.Open(FileName, Data::SAM_READ, Data::SAP_SEQUENTIAL) &&
		LoadNodesFromSCN(File, RootNode, PreloadResources);
}
//---------------------------------------------------------------------

}
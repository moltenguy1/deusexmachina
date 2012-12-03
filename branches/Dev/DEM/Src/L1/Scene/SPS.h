#pragma once
#ifndef __DEM_L1_SCENE_SPS_H__
#define __DEM_L1_SCENE_SPS_H__

#include <Scene/Mesh.h>
#include <Scene/Light.h>
#include <Scene/SceneNode.h>
#include <Data/QuadTree.h>
#include <kernel/ndebug.h>

// Scene spatial partitioning structure stuff

namespace Scene
{
struct CSPSRecord;

struct CSPSCell
{
	typedef CSPSRecord CElement;

	nArray<CElement> Meshes;
	nArray<CElement> Lights;

	CElement*	Add(const CSPSRecord& Object);
	bool		Remove(const CSPSRecord& Object); // By value
	void		RemoveElement(CElement* pElement); // By iterator
};

typedef Data::CQuadTree<CSPSRecord, CSPSCell> CSPS;
typedef CSPS::CNode CSPSNode;

class CSceneNodeAttr;

struct CSPSRecord
{
	CSceneNodeAttr&	Attr;
	bbox3			GlobalBox;
	CSPSNode*		pSPSNode;

	CSPSRecord(CSceneNodeAttr& NodeAttr): Attr(NodeAttr), pSPSNode(NULL) {} 
	CSPSRecord(const CSPSRecord& Rec): Attr(Rec.Attr), GlobalBox(Rec.GlobalBox), pSPSNode(Rec.pSPSNode) {} 

	bool		IsMesh() const { return Attr.IsA(CMesh::RTTI); }
	bool		IsLight() const { return Attr.IsA(CLight::RTTI); }

	CSPSRecord&	GetObject() { return *this; }
	void		GetCenter(vector2& Out) const;
	void		GetHalfSize(vector2& Out) const;
	CSPSNode*	GetQuadTreeNode() const { return pSPSNode; }
	void		SetQuadTreeNode(CSPSNode* pNode) { pSPSNode = pNode; }

	CSPSRecord&	operator =(const CSPSRecord& Other);
	bool		operator ==(const CSPSRecord& Other) const { return &Attr == &Other.Attr; }
	bool		operator !=(const CSPSRecord& Other) const { return &Attr != &Other.Attr; }
};

inline void CSPSRecord::GetCenter(vector2& Out) const
{
	n_assert(Attr.GetNode());
	const vector3& Pos = Attr.GetNode()->GetWorldMatrix().pos_component();
	Out.x = Pos.x;
	Out.y = Pos.z;
}
//---------------------------------------------------------------------

inline void CSPSRecord::GetHalfSize(vector2& Out) const
{
	Out.x = (GlobalBox.vmax.x - GlobalBox.vmin.x) * 0.5f;
	Out.y = (GlobalBox.vmax.z - GlobalBox.vmin.z) * 0.5f;
}
//---------------------------------------------------------------------

inline CSPSRecord& CSPSRecord::operator =(const CSPSRecord& Other)
{
	Attr = Other.Attr;
	GlobalBox = Other.GlobalBox;
	pSPSNode = Other.pSPSNode;
	return *this;
}
//---------------------------------------------------------------------

inline CSPSCell::CElement* CSPSCell::Add(const CSPSRecord& Object)
{
	if (Object.IsMesh()) return &Meshes.Append(Object);
	if (Object.IsLight()) return &Lights.Append(Object);
	n_assert_dbg(false);
	return NULL;
}
//---------------------------------------------------------------------

// Remove by value
inline bool CSPSCell::Remove(const CSPSRecord& Object)
{
	if (Object.IsMesh()) return Meshes.EraseElement(Object);
	if (Object.IsLight()) return Lights.EraseElement(Object);
	FAIL;
}
//---------------------------------------------------------------------

// Remove by iterator
inline void CSPSCell::RemoveElement(CSPSCell::CElement* pElement)
{
	if (!pElement) return;
	if (pElement->IsMesh()) Meshes.Erase(pElement);
	else if (pElement->IsLight()) Lights.Erase(pElement);
}
//---------------------------------------------------------------------

}

#endif

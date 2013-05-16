#pragma once
#ifndef __DEM_L2_PROP_SCENE_NODE_H__
#define __DEM_L2_PROP_SCENE_NODE_H__

#include <Game/Property.h>
#include <Scene/SceneNode.h>
#include <mathlib/bbox.h>

// Scene node property associates entity transform with scene graph node

namespace Properties
{

class CPropSceneNode: public Game::CProperty
{
	__DeclareClass(CPropSceneNode);
	__DeclarePropertyStorage;

protected:

	Scene::PSceneNode	Node;
	bool				ExistingNode;

public:

	//virtual ~CPropSceneNode() {}

	virtual void		Activate();
	virtual void		Deactivate();

	Scene::CSceneNode*	GetNode() const { return Node.GetUnsafe(); }
	void				GetAABB(bbox3& OutBox) const;
};

__RegisterClassInFactory(CPropSceneNode);

}

#endif

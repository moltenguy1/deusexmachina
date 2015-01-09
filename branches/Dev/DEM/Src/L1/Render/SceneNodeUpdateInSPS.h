#pragma once
#ifndef __DEM_L1_SNV_UPDATE_IN_SPS_H__
#define __DEM_L1_SNV_UPDATE_IN_SPS_H__

#include <Scene/NodeVisitor.h>
#include <Data/Array.h>

// Scene traversal that updates render object attributes' spatial information

namespace Render
{
class CSPS;

class CSceneNodeUpdateInSPS: public Scene::INodeVisitor
{
public:

	CSPS* pSPS;

	CSceneNodeUpdateInSPS(): pSPS(NULL) {} 

	virtual bool Visit(Scene::CSceneNode& Node);
};

}

#endif

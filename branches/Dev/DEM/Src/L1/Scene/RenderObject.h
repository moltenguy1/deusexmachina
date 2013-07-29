#pragma once
#ifndef __DEM_L1_SCENE_RENDER_OBJECT_H__
#define __DEM_L1_SCENE_RENDER_OBJECT_H__

#include <Scene/NodeAttribute.h>

// Base attribute class for any renderable scene objects, like
// regular models, particle systems, terrain patches etc.

namespace Scene
{

class CRenderObject: public CNodeAttribute
{
	__DeclareClassNoFactory;

public:

	virtual bool ValidateResources() = 0;
};

typedef Ptr<CRenderObject> PRenderObject;

}

#endif

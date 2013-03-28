#pragma once
#ifndef __DEM_L2_PROP_ANIM_H__
#define __DEM_L2_PROP_ANIM_H__

#include <Game/Property.h>
#include <Animation/MocapClip.h>
#include <DB/AttrID.h>

// Animation property manages node animation controllers, clip playback and blending

namespace Attr
{
	DeclareString(AnimDesc);
}

namespace Scene
{
	class CSceneNode;
	class CAnimController;
}

namespace Properties
{

class CPropAnimation: public Game::CProperty
{
	DeclareRTTI;
	DeclareFactory(CPropAnimation);
	DeclarePropertyStorage;
	DeclarePropertyPools(Game::LivePool);

private:

	struct CAnimTask
	{
		CStrID				ClipID;
		Anim::PMocapClip	Clip;

		nArray<Scene::CAnimController*> Ctlrs;

		float				Speed;
		float				FadeInTime;
		float				FadeOutTime;
		//DWORD Priority, float Weight

		float				CurrTime;

		bool				Loop;
		bool				IsPaused;
	};

	nDictionary<CStrID, Anim::PMocapClip>			Clips;
	nDictionary<Anim::CBoneID, Scene::CSceneNode*>	Nodes; //???where to store blend controller ref?

	nArray<CAnimTask>								Tasks;
	// Anim task list with IDs. Dictionary or array of slots with freeing slots.
	// Can grow array by 1 and allocate 1 slot at the beginning, since it is not
	// too frequent operation to add new animation tasks.

	void			AddChildrenToMapping(Scene::CSceneNode* pNode);

	DECLARE_EVENT_HANDLER(OnPropsActivated, OnPropsActivated);
	DECLARE_EVENT_HANDLER(OnBeginFrame, OnBeginFrame); //???OnMoveBefore?

public:

	virtual void	GetAttributes(nArray<DB::CAttrID>& Attrs);
	virtual void	Activate();
	virtual void	Deactivate();

	int				StartAnim(CStrID ClipID, bool Loop, float Offset, float Speed, DWORD Priority, float Weight, float FadeInTime, float FadeOutTime);
	void			PauseAnim(DWORD TaskID, bool Pause);
	void			StopAnim(DWORD TaskID, float FadeOutTime);
};

RegisterFactory(CPropAnimation);

}

#endif

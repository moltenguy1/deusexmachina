#include "KeyframeClip.h"

#include <Animation/NodeControllerKeyframe.h>

namespace Anim
{
__ImplementResourceClass(Anim::CKeyframeClip, 'KCLP', Anim::CAnimClip);

bool CKeyframeClip::Setup(const nArray<CKeyframeTrack>& _Tracks, const nArray<CStrID>& TrackMapping, float Length)
{
	if (State == Resources::Rsrc_Loaded) Unload();

	Tracks = _Tracks;

	Duration = Length;

	for (int i = 0; i < Tracks.GetCount(); ++i)
	{
		CSampler& Sampler = Samplers.GetOrAdd(TrackMapping[i]);
		switch (Tracks[i].Channel)
		{
			case Chnl_Translation:	Sampler.pTrackT = &Tracks[i]; break;
			case Chnl_Rotation:		Sampler.pTrackR = &Tracks[i]; break;
			case Chnl_Scaling:		Sampler.pTrackS = &Tracks[i]; break;
			default: n_error("CKeyframeClip::Setup() -> Unsupported channel for an SRT sampler track!");
		};
	}

	State = Resources::Rsrc_Loaded;
	OK;
}
//---------------------------------------------------------------------

void CKeyframeClip::Unload()
{
	State = Resources::Rsrc_NotLoaded;

	//!!!sampler pointers will become invalid! use smartptrs?
	// or in controller store clip ptr and clear sampler if clip becomes unloaded

	Samplers.Clear();
	Tracks.Clear();
}
//---------------------------------------------------------------------

Scene::PNodeController CKeyframeClip::CreateController(DWORD SamplerIdx) const
{
	Anim::PNodeControllerKeyframe Ctlr = n_new(Anim::CNodeControllerKeyframe);
	Ctlr->SetSampler(&Samplers.ValueAtIndex(SamplerIdx));
	return Ctlr.GetUnsafe();
}
//---------------------------------------------------------------------

}

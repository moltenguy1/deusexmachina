#include "NodeControllerKeyframe.h"

#include <Animation/KeyframeTrack.h>

namespace Anim
{

void CNodeControllerKeyframe::SetSampler(const CSampler* _pSampler)
{
	n_assert(_pSampler); //???allow NULL?
	pSampler = _pSampler;
	Channels.ClearAll();
	if (pSampler->pTrackT) Channels.Set(Scene::Chnl_Translation);
	if (pSampler->pTrackR) Channels.Set(Scene::Chnl_Rotation);
	if (pSampler->pTrackS) Channels.Set(Scene::Chnl_Scaling);
}
//---------------------------------------------------------------------

void CNodeControllerKeyframe::Clear()
{
	Channels.ClearAll();
	pSampler = NULL;
	Time = 0.f;
}
//---------------------------------------------------------------------

bool CNodeControllerKeyframe::ApplyTo(Math::CTransformSRT& DestTfm)
{
	if (!pSampler || (!pSampler->pTrackT && !pSampler->pTrackR && !pSampler->pTrackS)) FAIL;

	if (pSampler->pTrackT) ((CKeyframeTrack*)pSampler->pTrackT)->Sample(Time, DestTfm.Translation);
	if (pSampler->pTrackR) ((CKeyframeTrack*)pSampler->pTrackR)->Sample(Time, DestTfm.Rotation);
	if (pSampler->pTrackS) ((CKeyframeTrack*)pSampler->pTrackS)->Sample(Time, DestTfm.Scale);

	OK;
}
//---------------------------------------------------------------------

}

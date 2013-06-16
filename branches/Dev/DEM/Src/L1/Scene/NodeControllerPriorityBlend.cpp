#include "NodeControllerPriorityBlend.h"

#include <Scene/SceneNode.h>

namespace Scene
{

bool CNodeControllerPriorityBlend::ApplyTo(Math::CTransformSRT& DestTfm)
{
	if (!Sources.GetCount() || !Channels.IsAny(Chnl_Scaling | Chnl_Rotation | Chnl_Translation)) FAIL;

	bool HasActiveSrcs = false;
	for (int i = 0; i < Sources.GetCount(); ++i)
	{
		CSource& Src = Sources[i];
		if (Src.Ctlr->IsActive() && Src.Ctlr->ApplyTo(Src.SRT))
		{
			HasActiveSrcs = true;

			// Possibly slow feature. The good thing is that it is rarely used.
			if (pNode && pNode->GetParent() && Src.Ctlr->IsLocalSpace() != IsLocalSpace())
			{
				n_assert2(!pNode->GetParent()->IsWorldMatrixDirty(), "Don't create 'world' -> 'local + world blended' controller chains!\n");

				//???need?
				if (!Src.Ctlr->HasChannel(Chnl_Scaling)) Src.SRT.Scale.set(1.f, 1.f, 1.f);
				if (!Src.Ctlr->HasChannel(Chnl_Rotation)) Src.SRT.Rotation.set(0.f, 0.f, 0.f, 1.f);
				if (!Src.Ctlr->HasChannel(Chnl_Translation)) Src.SRT.Translation.set(0.f, 0.f, 0.f);

				matrix44 CurrMatrix;
				Src.SRT.ToMatrix(CurrMatrix);

				if (Src.Ctlr->IsLocalSpace())
				{
					CurrMatrix.mult_simple(pNode->GetParent()->GetWorldMatrix());
					Src.SRT.FromMatrix(CurrMatrix);
				}
				else
				{
					matrix44 InvParentPos;
					pNode->GetParent()->GetWorldMatrix().invert_simple(InvParentPos);
					InvParentPos.mult_simple(CurrMatrix);
					Src.SRT.FromMatrix(InvParentPos);
				}
			}
		}
	}

	if (!HasActiveSrcs) FAIL;

	if (HasChannel(Chnl_Scaling))
	{
		DestTfm.Scale.set(0.f, 0.f, 0.f);
		float TotalWeight = 0.f;
		for (int i = 0; i < Sources.GetCount(); ++i)
		{
			CSource& Src = Sources[i];
			if (!Src.Ctlr->IsActive() || !Src.Ctlr->HasChannel(Chnl_Scaling) || Src.Weight == 0.f) continue;

			float WeightLeft = 1.f - TotalWeight;
			bool TheLast = (Src.Weight >= WeightLeft);
			float Weight = TheLast ? WeightLeft : Src.Weight;
			DestTfm.Scale += Src.SRT.Scale * Weight;
			TotalWeight += Weight;

			if (TheLast) break;
		}

		if (TotalWeight == 0.f) DestTfm.Scale.set(1.f, 1.f, 1.f);
	}

	if (HasChannel(Chnl_Rotation))
	{
		DestTfm.Rotation.set(0.f, 0.f, 0.f, 1.f);
		float TotalWeight = 0.f;
		for (int i = 0; i < Sources.GetCount(); ++i)
		{
			CSource& Src = Sources[i];
			if (!Src.Ctlr->IsActive() || !Src.Ctlr->HasChannel(Chnl_Rotation) || Src.Weight == 0.f) continue;

			float WeightLeft = 1.f - TotalWeight;
			bool TheLast = (Src.Weight >= WeightLeft);
			float Weight = TheLast ? WeightLeft : Src.Weight;

			//!!!Need quaternion interpolation that support more than one quaternion! nlerp, squad?
			if (TotalWeight > 0.f)
			{
				//???can write inplace slerp?
				quaternion Tmp = DestTfm.Rotation;
				DestTfm.Rotation.slerp(Src.SRT.Rotation, Tmp, TotalWeight / (TotalWeight + Weight));
			}
			else DestTfm.Rotation = Src.SRT.Rotation;

			if (TheLast) break;

			TotalWeight += Weight;
		}
	}

	if (HasChannel(Chnl_Translation))
	{
		DestTfm.Translation.set(0.f, 0.f, 0.f);
		float TotalWeight = 0.f;
		for (int i = 0; i < Sources.GetCount(); ++i)
		{
			CSource& Src = Sources[i];
			if (!Src.Ctlr->IsActive() || !Src.Ctlr->HasChannel(Chnl_Translation) || Src.Weight == 0.f) continue;

			float WeightLeft = 1.f - TotalWeight;
			bool TheLast = (Src.Weight >= WeightLeft);
			float Weight = TheLast ? WeightLeft : Src.Weight;
			DestTfm.Translation += Src.SRT.Translation * Weight;

			if (TheLast) break;

			TotalWeight += Weight;
		}
	}

	OK;
}
//---------------------------------------------------------------------

}

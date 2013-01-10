#include "ModelRenderer.h"

#include <Scene/Model.h>
#include <Render/RenderServer.h>
#include <Data/Params.h>

namespace Render
{
ImplementRTTI(Render::IModelRenderer, Render::IRenderer);

// Forward rendering:
// - Render solid objects to depth buffer, front to back (only if render to texture?)
// - Render atest objects to depth buffer, front to back (only if render to texture?)
// - Occlusion (against z-buffer filled by 1 and 2)
// - Render sky without zwrite and mb without ztest //???better to render sky after all other non-alpha/additive geometry?
// - Render terrain (lightmapped/unlit/...?) FTB //???render after all opaque except skybox?
// - Render opaque geometry (static, skinned, blended, envmapped) FTB
// - Render alpha-tested geometry (static, leaf, tree) FTB
// - Render alpha-blended geometry (alpha, alpha_soft, skinned_alpha, env_alpha, water) BTF
// - Render particles (alpha, then additive) BTF?
// - HDR

//!!!rendering front-to-back with existing z-buffer has no point!
//z-pass FtB has meaning, if pixel shader is not empty!

void IModelRenderer::Init(const Data::CParams& Desc)
{
	//???add AllowGrowInstanceBuffer or MaxInstanceCount or both?
	DWORD InitialInstanceCount = Desc.Get<int>(CStrID("InitialInstanceCount"), 0);
	if (InitialInstanceCount)
	{
		nArray<CVertexComponent> InstCmps(4, 0);
		for (int i = 0; i < 4; ++i)
		{
			CVertexComponent& Cmp = InstCmps[i];
			Cmp.Format = CVertexComponent::Float4;
			Cmp.Semantic = CVertexComponent::TexCoord;
			Cmp.Index = i + 1;
			Cmp.Stream = 1;
		}
		InstanceBuffer.Create();
		n_assert(InstanceBuffer->Create(RenderSrv->GetVertexLayout(InstCmps), InitialInstanceCount, UsageDynamic, AccessWrite));
	}
}
//---------------------------------------------------------------------

void IModelRenderer::AddRenderObjects(const nArray<Scene::CRenderObject*>& Objects)
{
	n_assert_dbg(BatchType.IsValid());

	for (int i = 0; i < Objects.Size(); ++i)
	{
		if (!Objects[i]->IsA(Scene::CModel::RTTI)) continue;
		Scene::CModel* pModel = (Scene::CModel*)Objects[i];

		n_assert_dbg(pModel->BatchType.IsValid());
		if (pModel->BatchType != BatchType) continue;

		//!!!in light renderers must collect lights here!

		// Find desired tech:
		// Get object feature flags (material + geometry)
		// Add renderer feature flags (mode, light in light renderers)
		// If renderer has optional flags, remove them in object's feature flags
		// Else if tech has optional flags, it handles them in shader dictionary (all possible combinations)

		DWORD FeatFlags = pModel->FeatureFlags | pModel->Material->GetFeatureFlags();

		CShader::HTech hTech = pModel->Material->GetShader()->GetTechByFeatures(FeatFlags);
		n_assert(hTech);

		CModelRecord* pRec = Models.Reserve(1);
		pRec->pModel = pModel;
		pRec->hTech = hTech;
	}
}
//---------------------------------------------------------------------

// NB: It is overriden to empty method in CModelRendererNoLight
void IModelRenderer::AddLights(const nArray<Scene::CLight*>& Lights)
{
	pLights = &Lights;
	//for (int i = 0; i < Lights.Size(); ++i)
	//{
	//	// Perform something with lights or just store array ref
	//}
}
//---------------------------------------------------------------------

}
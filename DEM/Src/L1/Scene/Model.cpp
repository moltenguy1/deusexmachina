#include "Model.h"

#include <Scene/Scene.h>
#include <Render/Renderer.h>
#include <Data/BinaryReader.h>

//!!!OLD!
#include "scene/nsceneserver.h"

namespace Render
{
	bool LoadMeshFromNVX2(const nString& FileName, PMesh OutMesh);
}

namespace Scene
{
ImplementRTTI(Scene::CModel, Scene::CSceneNodeAttr);
ImplementFactory(Scene::CModel);

bool CModel::LoadDataBlock(nFourCC FourCC, Data::CBinaryReader& DataReader)
{
	switch (FourCC)
	{
		case 'RDHS': // SHDR
		{
			CStrID ShaderID;
			DataReader.Read(ShaderID);

			SetShader(ShaderID.CStr());

			//Material = RenderSrv->MaterialMgr.GetResource(MaterialID);

			//!!!TMP!
			// For now - create material, find shader by ID and set to material
			//Material = n_new(Render::CMaterial(Renderer->GetShader(Value)));
			Material = RenderSrv->MaterialMgr.GetTypedResource(ShaderID);
			Material->SetShader(RenderSrv->ShaderMgr.GetTypedResource(ShaderID));

			OK;
		}
		case 'SRAV': // VARS
		{
			short Count;
			if (!DataReader.Read(Count)) FAIL;
			for (short i = 0; i < Count; ++i)
			{
				CStrID VarName;
				DataReader.Read(VarName);
				CShaderVar& Var = ShaderVars.Add(VarName);
				if (Material.isvalid()) Var.Bind(*Material->GetShader());
				DataReader.Read(Var.Value);
				//???check type if bound? use SetValue for it?
			}
			OK;
		}
		case 'SXET': // TEXS
		{
			short Count;
			if (!DataReader.Read(Count)) FAIL;
			for (short i = 0; i < Count; ++i)
			{
				CStrID VarName;
				DataReader.Read(VarName);
				CShaderVar& Var = ShaderVars.Add(VarName);
				if (Material.isvalid()) Var.Bind(*Material->GetShader());

				CStrID TextureID;
				DataReader.Read(TextureID);

				SetTexture(nShaderState::StringToParam(VarName.CStr()), TextureID.CStr());

				Var.Value = (Render::PTexture)RenderSrv->TextureMgr.GetTypedResource(TextureID);
			}
			OK;
		}
		case 'HSEM': // MESH
		{
			CStrID MeshID;
			DataReader.Read(MeshID);

			SetMesh(MeshID.CStr());

			Mesh = RenderSrv->MeshMgr.GetTypedResource(MeshID);

			OK;
		}
		case 'RGSM': // MSGR
		{
			return DataReader.Read(MeshGroupIndex);
		}
		default: FAIL;
	}
}
//---------------------------------------------------------------------

void CModel::OnRemove()
{
	if (pSPSRecord)
	{
		pNode->GetScene()->SPS.RemoveElement(pSPSRecord);
		pSPSRecord = NULL;
	}
}
//---------------------------------------------------------------------

void CModel::Update()
{
	if (!pSPSRecord)
	{
		CSPSRecord NewRec(*this);
		GetBox(NewRec.GlobalBox);
		pSPSRecord = pNode->GetScene()->SPS.AddObject(NewRec);
	}
	else if (pNode->IsWorldMatrixChanged()) //!!! || Group.LocalBox changed
	{
		GetBox(pSPSRecord->GlobalBox);
		pNode->GetScene()->SPS.UpdateElement(pSPSRecord);
	}
}
//---------------------------------------------------------------------

//!!!differ between CalcBox - primary source, and GetBox - return cached box from spatial record!
//???inline?
void CModel::GetBox(bbox3& OutBox) const
{
	// If local params changed, recompute AABB
	// If transform of host node changed, update global space AABB (rotate, scale)
	OutBox = refMesh->Group(MeshGroupIndex).Box;
	OutBox.transform(pNode->GetWorldMatrix());
}
//---------------------------------------------------------------------

//!!!
// ==================== OLD ===========================================
//!!!

void CModel::SetTexture(nShaderState::Param param, const char* texName)
{
    n_assert(texName);

    // silently ignore invalid parameters
    if (nShaderState::InvalidParameter == param)
    {
        n_printf("WARNING: invalid shader parameter\n");
        return;
    }

    // see if texture variable already exists
    int i;
    for (i = 0; i < texNodeArray.Size(); i++)
        if (this->texNodeArray[i].shaderParameter == param) break;
    if (i == texNodeArray.Size())
    {
        // add new texnode to array
        CTextureNode newTexNode(param, texName);
        this->texNodeArray.Append(newTexNode);
    }
    else
    {
        // invalidate existing texture
        this->UnloadTexture(i);
        this->texNodeArray[i].texName = texName;
    }
    // flag to load resources
    this->resourcesValid = false;
}
//---------------------------------------------------------------------

const char* CModel::GetTexture(nShaderState::Param param) const
{
    for (int i = 0; i < texNodeArray.Size(); i++)
        if (texNodeArray[i].shaderParameter == param)
            return texNodeArray[i].texName.Get();
    return 0;
}
//---------------------------------------------------------------------

void CModel::UnloadShader()
{
    if (refShader.isvalid())
    {
        refShader->Release();
        refShader.invalidate();
    }
}
//---------------------------------------------------------------------

bool CModel::LoadShader()
{
    n_assert(!shaderName.IsEmpty());

    if (!refShader.isvalid())
    {
        const CFrameShader* pFrameShader = nSceneServer::Instance()->GetRenderPath();
        n_assert(pFrameShader);
        int RPShaderIdx = pFrameShader->FindShaderIndex(shaderName);
        n_assert(-1 != RPShaderIdx);
        const nRpShader& rpShader = pFrameShader->shaders[RPShaderIdx];
        shaderIndex = rpShader.GetBucketIndex();
        refShader = rpShader.GetShader();
        refShader->AddRef();
    }
    return true;
}
//---------------------------------------------------------------------

void CModel::UnloadTexture(int index)
{
	CTextureNode& texNode = texNodeArray[index];
	if (texNode.refTexture.isvalid())
	{
		texNode.refTexture->Release();
		texNode.refTexture.invalidate();
	}
}
//---------------------------------------------------------------------

bool CModel::LoadTexture(int index)
{
    CTextureNode& texNode = texNodeArray[index];
    if ((!texNode.refTexture.isvalid()) && (!texNode.texName.IsEmpty()))
    {
        // load only if the texture is used in the shader
        if (IsTextureUsed(texNode.shaderParameter))
        {
            nTexture2* tex = nGfxServer2::Instance()->NewTexture(texNode.texName);
            n_assert(tex);
            if (!tex->IsLoaded())
            {
                tex->SetFilename(texNode.texName);
                if (!tex->Load())
                {
                    n_printf("nMaterialNode: Error loading texture '%s'\n", texNode.texName.Get());
                    return false;
                }
            }
            texNode.refTexture = tex;
            shaderParams.SetArg(texNode.shaderParameter, nShaderArg(tex));
        }
    }
    return true;
}
//---------------------------------------------------------------------

void CModel::UnloadMesh()
{
	if (refMesh.isvalid())
	{
		refMesh->Release();
		refMesh.invalidate();
	}
}
//---------------------------------------------------------------------

// Load new mesh, release old one if valid. Also initializes the groupIndex member.
bool CModel::LoadMesh()
{
    if (!refMesh.isvalid() && meshName.IsValid())
    {
        // append mesh usage to mesh resource name
        nString resourceName;
        resourceName.Format("%s_%d", meshName.Get(), meshUsage);

        // get a new or shared mesh
        nMesh2* mesh = nGfxServer2::Instance()->NewMesh(resourceName);
        n_assert(mesh);
        if (!mesh->IsLoaded())
        {
            mesh->SetFilename(meshName);
            mesh->SetUsage(meshUsage);

            if (!mesh->Load())
            {
                n_printf("nMeshNode: Error loading mesh '%s'\n", meshName.Get());
                mesh->Release();
                return false;
            }
        }
        refMesh = mesh;
        //SetLocalBox(refMesh->Group(groupIndex).Box);
    }
    return true;
}
//---------------------------------------------------------------------

bool CModel::LoadResources()
{
	//if (!nTransformNode::LoadResources()) return false;
	if (!LoadShader()) return false;
	for (int i = 0; i < texNodeArray.Size(); i++)
		if (!LoadTexture(i)) return false;
	if (!LoadMesh()) return false;
	if (!Render::LoadMeshFromNVX2(meshName, Mesh)) return false; //!!!usage & access!
	return true;
}
//---------------------------------------------------------------------

void CModel::UnloadResources()
{
    //nTransformNode::UnloadResources();
    for (int i = 0; i < texNodeArray.Size(); i++)
		UnloadTexture(i);
    UnloadShader();
	UnloadMesh();
}
//---------------------------------------------------------------------

}
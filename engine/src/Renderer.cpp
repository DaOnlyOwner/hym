#include "Renderer.h"
#include "Debug.h"
#include "Components.h"
#include "Graphics/GraphicsTools/interface/MapHelper.hpp"
#include "BasicMath.hpp"

void Hym::Renderer::Init(Scene& scene, int probesX, int probesY, int probesZ, int raysPerProbe)
{
    /*BufferDesc sunDesc;
    sunDesc.BindFlags = BIND_UNIFORM_BUFFER;
    sunDesc.Name = "Sun Buffer";
    sunDesc.Usage = USAGE_DYNAMIC;
    sunDesc.Size = sizeof(Sun);
    sunDesc.CPUAccessFlags = CPU_ACCESS_WRITE;
    this->device->CreateBuffer(sunDesc, nullptr, &sunBuffer);*/
    irrField.Init(probesX, probesY, probesZ, raysPerProbe,scene);
    initGeometryPSO(scene);
    initRenderTextures();
    initCompositePSO(scene);
}

Hym::dl::SamplerDesc Hym::Renderer::createAnisoSampler()
{
    SamplerDesc sampler{
        FILTER_TYPE_ANISOTROPIC, FILTER_TYPE_ANISOTROPIC, FILTER_TYPE_ANISOTROPIC,
        TEXTURE_ADDRESS_WRAP, TEXTURE_ADDRESS_WRAP, TEXTURE_ADDRESS_WRAP, 0.f, 8 //
    };

    return sampler;
   
}

Hym::dl::SamplerDesc Hym::Renderer::createLinearSampler()
{
    SamplerDesc sampler{
        FILTER_TYPE_LINEAR,FILTER_TYPE_LINEAR,FILTER_TYPE_LINEAR,
        TEXTURE_ADDRESS_CLAMP,TEXTURE_ADDRESS_CLAMP,TEXTURE_ADDRESS_CLAMP
    };
    return sampler;
}

void Hym::Renderer::RebuildSRBs(Scene& scene)
{   
    //gen_views(manager.GetNormalTexs(), "normalTexs", SHADER_TYPE_PIXEL);
    initGeometrySRB(scene);
}

//void Hym::Renderer::InitIrrField(int pX, int pY, int pZ, int raysPerProbe, const glm::vec3& minScene, const glm::vec3& maxScene)
//{
//    irrField.Init(pX, pY, pZ, raysPerProbe, minScene, maxScene, sunBuffer);
//}

//void Hym::Renderer::InitTLAS()
//{
//    compositeSRB->GetVariableByName(SHADER_TYPE_PIXEL, "tlas")
//        ->Set(Resources::Inst().GetTLAS());
//}

void Hym::Renderer::Draw(Scene& scene, Camera& cam)
{
    auto& res = scene.GetResourceManager();
    entt::registry& reg = scene.GetRegistry();
    auto group = reg.group<TransformComponent,ModelComponent>();

    scene.GetSun().Set();
    irrField.Draw();

    ITextureView* RTVs[] =
    {
        gbuffer.albedoBuffer->GetDefaultView(TEXTURE_VIEW_RENDER_TARGET),
        gbuffer.normalBuffer->GetDefaultView(TEXTURE_VIEW_RENDER_TARGET)
    };

    ITextureView* DSV = gbuffer.depthBuffer->GetDefaultView(TEXTURE_VIEW_DEPTH_STENCIL);
    Imm->SetRenderTargets(_countof(RTVs), RTVs, DSV, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    const float clearColor[] = { 0,0,0,1 };

    for (int i = 0; i < _countof(RTVs); i++)
    {
        Imm->ClearRenderTarget(RTVs[i], clearColor, RESOURCE_STATE_TRANSITION_MODE_NONE);
    }

    Imm->ClearDepthStencil(DSV, CLEAR_DEPTH_FLAG, 1.f, 0, RESOURCE_STATE_TRANSITION_MODE_NONE);
    auto projM = cam.GetProjMatrix();
    auto viewM = cam.GetViewMatrix();

    Imm->SetPipelineState(&geometryPass.GetPSO());    
    Imm->CommitShaderResources(geomSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    auto vbuffer = res.GetVertexBuffer().GetBuffer();
    auto ibuffer = res.GetIndexBuffer().GetBuffer();
    Imm->SetIndexBuffer(ibuffer, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    for (auto e : group)
    {
        auto [trans, model] = group.get<TransformComponent, ModelComponent>(e);
        auto& mesh = model.mesh;
        IBuffer* vbos[] = { vbuffer };
        u64 offset[] = { mesh.offsetVertex * sizeof(Vertex) };
        Imm->SetVertexBuffers(0, 1, vbos, offset, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);

        {
            auto con = geomConstants.Map();
            auto [modelM, normal] = trans.GetModel_Normal();
            con->model = glm::transpose(modelM);
            con->normal = glm::transpose(normal);
            con->MVP = glm::transpose(projM * viewM * modelM);
            con->matIdx = model.matIdx;
        }

        DrawIndexedAttribs attrs;
        attrs.IndexType = VT_UINT32;
        attrs.NumIndices = mesh.numIndices;
        attrs.FirstIndexLocation = mesh.offsetIndex;
        attrs.Flags = DRAW_FLAG_VERIFY_ALL;
        Imm->DrawIndexed(attrs);
    }


    {
        auto view = viewBuffer.Map();
        view->eyePos = cam.GetEyePos();
        view->VPInv = glm::transpose(glm::inverse(cam.GetProjMatrix() * cam.GetViewMatrix()));
    }



    auto* rtv = SwapChain->GetCurrentBackBufferRTV();
    Imm->SetRenderTargets(1, &rtv, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    Imm->ClearRenderTarget(rtv, clearColor, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    Imm->SetPipelineState(&compositePass.GetPSO());
    Imm->CommitShaderResources(compositeSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    Imm->SetVertexBuffers(0, 0, nullptr, nullptr, RESOURCE_STATE_TRANSITION_MODE_NONE, SET_VERTEX_BUFFERS_FLAG_RESET);
    Imm->SetIndexBuffer(nullptr, 0, RESOURCE_STATE_TRANSITION_MODE_NONE);
    Imm->Draw(DrawAttribs{ 3,DRAW_FLAG_VERIFY_ALL });
}

void Hym::Renderer::Resize(Scene& scene)
{
    initRenderTextures();
    initCompositeSRB(scene);
}

//void Hym::Renderer::SetSun(const Sun& sun)
//{
//    this->sun = &sun;
//}

// Reference: https://github.com/DiligentGraphics/DiligentSamples/blob/master/Tutorials/Tutorial19_RenderPasses/src/Tutorial19_RenderPasses.cpp
void Hym::Renderer::initGeometryPSO(Scene& scene)
{
    auto& manager = scene.GetResourceManager();
    LayoutElement elems[] = {
        LayoutElement{0, 0, 3, VT_FLOAT32, False},
        //LayoutElement{1, 0, 1, VT_FLOAT32, false},
        //LayoutElement{1, 0, 3, VT_FLOAT32, False},
        //LayoutElement(2, 0, 3, VT_FLOAT32, False),
        LayoutElement(1, 0, 3, VT_FLOAT32, False),
        //LayoutElement(3, 0, 1, VT_FLOAT32, False),
        LayoutElement(2, 0, 2, VT_FLOAT32, False),
        //LayoutElement(3, 0, 1, VT_INT32, False)
        //LayoutElement(5, 0, 1, VT_FLOAT32, False),
        //LayoutElement(6, 0, 1, VT_FLOAT32, False)
    };

    /*ShaderResourceVariableDesc vars[] =
    { {SHADER_TYPE_VERTEX, "geomConst", SHADER_RESOURCE_VARIABLE_TYPE_STATIC},
        //{SHADER_TYPE_PIXEL, "texSampler",SHADER_RESOURCE_VARIABLE_TYPE_STATIC}
    };*/

    auto sdesc = createAnisoSampler();

    ImmutableSamplerDesc anisoSamplerImmutable[] =
    {
        {SHADER_TYPE_PIXEL, "texSampler",sdesc}
    };

    std::string albedoTexsSize = std::to_string(manager.GetAlbedoTexs().size());
    std::pair<const char*, const char*> macros[] =
    {
        {"NUM_ALBEDO_TEXS", albedoTexsSize.c_str()}
    };

    geometryPass
        .SetDefaultDeferred()
        .SetLayoutElems(ArrayRef<LayoutElement>::MakeRef(elems, _countof(elems)))
        .SetVS("Geometry VS", SHADER_RES "/geometry_vs.hlsl")
        .SetPS("Geometry PS", SHADER_RES "/geometry_ps.hlsl", ArrayRef<std::pair<const char*, const char*>>::MakeRef(macros,_countof(macros)))
        .SetName("Geometry Pipeline")
        .SetSamplers(ArrayRef<ImmutableSamplerDesc>::MakeRef(anisoSamplerImmutable,_countof(anisoSamplerImmutable)))
        //.SetShaderVars(ArrayRef<ShaderResourceVariableDesc>::MakeRef(vars,_countof(vars)))
        .Create();

    initGeometryBuffers();

    //auto& pso = geometryPass.GetPSO();
    //pso.GetStaticVariableByName(SHADER_TYPE_VERTEX, "geomConst")->Set(geomConstants.GetBuffer());

    initGeometrySRB(scene);

}

void Hym::Renderer::initGeometrySRB(Scene& scene)
{
    geomSRB.Release();
    geometryPass.GetPSO().CreateShaderResourceBinding(&geomSRB);
    auto& manager = scene.GetResourceManager();
    auto albedos = manager.GetAlbedoTexViews();
    geomSRB->GetVariableByName(SHADER_TYPE_PIXEL, "albedoTexs")->SetArray(albedos.data(), 0, albedos.size());
    //gen_views(albedos, "albedoTexs", SHADER_TYPE_PIXEL);
    geomSRB->GetVariableByName(SHADER_TYPE_PIXEL, "albedoTexs");
    geomSRB->GetVariableByName(SHADER_TYPE_PIXEL, "materials")->Set(manager.GetMaterialBuffer().GetBuffer()->GetDefaultView(BUFFER_VIEW_SHADER_RESOURCE));
    geomSRB->GetVariableByName(SHADER_TYPE_VERTEX, "geomConst")->Set(geomConstants.GetBuffer());
}

void Hym::Renderer::initGeometryBuffers()
{
    geomConstants = UniformBuffer<GeometryPassVSPerMeshConstants>("Geometry Constants Buffer");
}

void Hym::Renderer::initCompositePSO(Scene& scene)
{

    auto linearDesc = createLinearSampler();
    ImmutableSamplerDesc linearSamplerImmutable[] =
    {
        {SHADER_TYPE_PIXEL, "irradianceTex_sampler",linearDesc},
        {SHADER_TYPE_PIXEL, "weightTex_sampler",linearDesc}
    };

    compositePass
        .SetDefaultComposite()
        .SetVS("Composite VS", SHADER_RES "/composite_vs.hlsl")
        .SetPS("Composite PS", SHADER_RES "/composite_ps.hlsl")
        .SetSamplers(ArrayRef<ImmutableSamplerDesc>::MakeRef(linearSamplerImmutable,_countof(linearSamplerImmutable)))
        .Create();

    viewBuffer = UniformBuffer<View>("View Buffer");

    initCompositeSRB(scene);
}

void Hym::Renderer::initCompositeSRB(Scene& scene)
{
    compositeSRB.Release();
    auto& pso = compositePass.GetPSO();
    pso.CreateShaderResourceBinding(&compositeSRB);
    compositeSRB->GetVariableByName(SHADER_TYPE_PIXEL, "GBuffer_Albedo")
        ->Set(gbuffer.albedoBuffer->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE));
    compositeSRB->GetVariableByName(SHADER_TYPE_PIXEL, "GBuffer_Normal")
        ->Set(gbuffer.normalBuffer->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE));
    compositeSRB->GetVariableByName(SHADER_TYPE_PIXEL, "GBuffer_Depth")
        ->Set(gbuffer.depthBuffer->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE));
    /*compositeSRB->GetVariableByName(SHADER_TYPE_PIXEL, "sun")
        ->Set(sunBuffer);*/
    compositeSRB->GetVariableByName(SHADER_TYPE_PIXEL, "view")
        ->Set(viewBuffer.GetBuffer());
    compositeSRB->GetVariableByName(SHADER_TYPE_PIXEL, "sun")->Set(scene.GetSun().GetSunBuffer());
    compositeSRB->GetVariableByName(SHADER_TYPE_PIXEL, "tlas")->Set(scene.GetTLAS());
    compositeSRB->GetVariableByName(SHADER_TYPE_PIXEL, "irradianceTex")->Set(irrField.GetIrrTexView());
    compositeSRB->GetVariableByName(SHADER_TYPE_PIXEL, "weightTex")->Set(irrField.GetWeightTexView());
    compositeSRB->GetVariableByName(SHADER_TYPE_PIXEL, "L")->Set(irrField.GetLightFieldBuffer());
}

void Hym::Renderer::initRenderTextures()
{
    const auto& SCDesc = SwapChain->GetDesc();

    TextureDesc desc;
    desc.Name = "Albedo GBuffer";
    desc.Type = RESOURCE_DIM_TEX_2D;
    desc.BindFlags = BIND_RENDER_TARGET | BIND_SHADER_RESOURCE;
    desc.Format = DIFFUSE_FORMAT;
    desc.Width = SCDesc.Width;
    desc.Height = SCDesc.Height;
    gbuffer.albedoBuffer.Release();
    Dev->CreateTexture(desc, nullptr, &gbuffer.albedoBuffer);

    desc.Name = "Normal GBuffer";
    desc.Format = NORMAL_FORMAT;
    desc.BindFlags = BIND_RENDER_TARGET | BIND_SHADER_RESOURCE;
    gbuffer.normalBuffer.Release();
    Dev->CreateTexture(desc, nullptr, &gbuffer.normalBuffer);

    //desc.Name = "Position GBuffer";
    //gbuffer.positionBuffer.Release();
    //device->CreateTexture(desc, nullptr, &gbuffer.positionBuffer);

    desc.Name = "Depth GBuffer";
    desc.BindFlags = BIND_DEPTH_STENCIL | BIND_SHADER_RESOURCE;
    desc.Format = DEPTH_FORMAT;
    gbuffer.depthBuffer.Release();
    Dev->CreateTexture(desc, nullptr, &gbuffer.depthBuffer);
}


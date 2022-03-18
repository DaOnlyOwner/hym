#include "Renderer.h"
#include "Debug.h"
#include "Components.h"
#include "Graphics/GraphicsTools/interface/MapHelper.hpp"


void Hym::Renderer::Init()
{
    /*BufferDesc sunDesc;
    sunDesc.BindFlags = BIND_UNIFORM_BUFFER;
    sunDesc.Name = "Sun Buffer";
    sunDesc.Usage = USAGE_DYNAMIC;
    sunDesc.Size = sizeof(Sun);
    sunDesc.CPUAccessFlags = CPU_ACCESS_WRITE;
    this->device->CreateBuffer(sunDesc, nullptr, &sunBuffer);*/

    initGeometryPSO();
    initRenderTextures();
    initCompositePSO();

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
    auto group = reg.group<TransformComponent>(entt::get<ModelComponent>);

    /*MapHelper<Sun> mapHelper;
    mapHelper.Map(imm, sunBuffer, MAP_WRITE, MAP_FLAG_DISCARD);
    mapHelper->direction = sun->direction;
    mapHelper->color = sun->color;
    mapHelper.Unmap();

    irrField.Draw();*/

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
            auto [model, normal] = trans.GetModel_Normal();
            con->model = glm::transpose(model);
            con->normal = glm::transpose(normal);
            con->MVP = glm::transpose(projM * viewM * model);
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


    EndFrame();
}

void Hym::Renderer::EndFrame()
{
    Imm->Flush();
    Imm->FinishFrame();
    SwapChain->Present();
}

void Hym::Renderer::Resize()
{
    initRenderTextures();
    initCompositeSRB();
}

//void Hym::Renderer::SetSun(const Sun& sun)
//{
//    this->sun = &sun;
//}

// Reference: https://github.com/DiligentGraphics/DiligentSamples/blob/master/Tutorials/Tutorial19_RenderPasses/src/Tutorial19_RenderPasses.cpp
void Hym::Renderer::initGeometryPSO()
{
    LayoutElement elems[] = {
        LayoutElement{0, 0, 3, VT_FLOAT32, False},
        //LayoutElement{1, 0, 1, VT_FLOAT32, false},
        //LayoutElement{1, 0, 3, VT_FLOAT32, False},
        //LayoutElement(2, 0, 3, VT_FLOAT32, False),
        LayoutElement(1, 0, 3, VT_FLOAT32, False),
        //LayoutElement(3, 0, 1, VT_FLOAT32, False),
        LayoutElement(2, 0, 2, VT_FLOAT32, False),
        //LayoutElement(5, 0, 1, VT_FLOAT32, False),
        //LayoutElement(6, 0, 1, VT_FLOAT32, False)
    };

    ShaderResourceVariableDesc vars[] =
    { {
        SHADER_TYPE_VERTEX, "geomConst", SHADER_RESOURCE_VARIABLE_TYPE_STATIC
    } };

    geometryPass
        .SetDefaultDeferred()
        .SetLayoutElems(ArrayRef<LayoutElement>::MakeRef(elems, _countof(elems)))
        .SetVS("Geometry VS", SHADER_RES "/geometry_vs.hlsl")
        .SetPS("Geometry PS", SHADER_RES "/geometry_ps.hlsl")
        .SetName("Geometry Pipeline")
        .SetShaderVars(ArrayRef<ShaderResourceVariableDesc>::MakeRef(vars,_countof(vars)))
        .Create();

    initGeometryBuffers();
    initGeometrySRB();

}

void Hym::Renderer::initGeometrySRB()
{
    auto& pso = geometryPass.GetPSO();
    auto i = pso.GetStaticVariableCount(SHADER_TYPE_VERTEX);
    pso.GetStaticVariableByName(SHADER_TYPE_VERTEX, "geomConst")->Set(geomConstants.GetBuffer());
    pso.CreateShaderResourceBinding(&geomSRB, true);
}

void Hym::Renderer::initGeometryBuffers()
{
    geomConstants = UniformBuffer<GeometryPassVSPerMeshConstants>("Geometry Constants Buffer");
}

void Hym::Renderer::initCompositePSO()
{
    compositePass
        .SetDefaultComposite()
        .SetVS("Composite VS", SHADER_RES "/composite_vs.hlsl")
        .SetPS("Composite PS", SHADER_RES "/composite_ps.hlsl")
        .Create();

    viewBuffer = UniformBuffer<View>("View Buffer");

    initCompositeSRB();
}

void Hym::Renderer::initCompositeSRB()
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


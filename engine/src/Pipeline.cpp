#include "Pipeline.h"
#include "Renderer.h"
#include "Debug.h"
#include "Texture.h"
#include "ShaderMacroHelper.hpp"

Hym::Pipeline::Pipeline()
{
}

Hym::Pipeline& Hym::Pipeline::SetCullmode(CULL_MODE cm)
{
	ci.GraphicsPipeline.RasterizerDesc.CullMode = cm;
    return *this;
}

Hym::Pipeline& Hym::Pipeline::SetDepthTesting(bool enable)
{
	ci.GraphicsPipeline.DepthStencilDesc.DepthEnable = enable;
    return *this;
}

Hym::Pipeline& Hym::Pipeline::SetLayoutElems(const ArrayRef<LayoutElement>& a)
{
    ci.GraphicsPipeline.InputLayout.LayoutElements = a.data;
    ci.GraphicsPipeline.InputLayout.NumElements = a.size;
    return *this;
}

Hym::Pipeline& Hym::Pipeline::SetVS(const char* name, const char* filename, const ArrayRef<std::pair<const char*, const char*>>& macros)
{
    ShaderCreateInfo sci;
    sci.SourceLanguage = SHADER_SOURCE_LANGUAGE_HLSL;
    //sci.UseCombinedTextureSamplers = true;
    sci.ShaderCompiler = SHADER_COMPILER_DXC;

    ShaderMacroHelper helper;

    for (int i = 0; i < macros.size; i++)
    {
        auto& macro = macros.data[i];
        helper.AddShaderMacro(macro.first, macro.second);
    }

    helper.Finalize();

    if (macros.size != 0)
        sci.Macros = helper;
    
    sci.pShaderSourceStreamFactory = ShaderStream;
    {
        sci.Desc.ShaderType = SHADER_TYPE_VERTEX;
        sci.EntryPoint = "main";
        sci.Desc.Name = name;
        sci.FilePath = filename;
        Dev->CreateShader(sci, &vs);
    }


    ci.pVS = vs;
    return *this;
}

Hym::Pipeline& Hym::Pipeline::SetPS(const char* name, const char* filename, const ArrayRef<std::pair<const char*, const char*>>& macros)
{
    ShaderCreateInfo ShaderCI;
    ShaderCI.SourceLanguage = SHADER_SOURCE_LANGUAGE_HLSL;
    //ShaderCI.UseCombinedTextureSamplers = true;
    ShaderCI.ShaderCompiler = SHADER_COMPILER_DXC;

    ShaderMacroHelper helper;

    for (int i = 0; i < macros.size; i++)
    {
        auto& macro = macros.data[i];
        helper.AddShaderMacro(macro.first, macro.second);
    }

    helper.Finalize();

    if (macros.size != 0)
        ShaderCI.Macros = helper;
    
    ShaderCI.pShaderSourceStreamFactory = ShaderStream;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_PIXEL;
        ShaderCI.EntryPoint = "main";
        ShaderCI.Desc.Name = name;
        ShaderCI.FilePath = filename;
        Dev->CreateShader(ShaderCI, &ps);
    }


    ci.pPS = ps;
    return *this;
}

Hym::Pipeline& Hym::Pipeline::SetShaderVars(const ArrayRef<ShaderResourceVariableDesc>& a)
{
    ci.PSODesc.ResourceLayout.Variables = a.data;
    ci.PSODesc.ResourceLayout.NumVariables = a.size;
    return *this;
}

Diligent::GraphicsPipelineStateCreateInfo& Hym::Pipeline::GetPSOCi()
{
    return ci;
}

Hym::Pipeline& Hym::Pipeline::SetSamplers(const ArrayRef<ImmutableSamplerDesc>& a)
{
    this->ci.PSODesc.ResourceLayout.ImmutableSamplers = a.data;
    this->ci.PSODesc.ResourceLayout.NumImmutableSamplers = a.size;
    return *this;
}

Hym::Pipeline& Hym::Pipeline::SetDefaultDeferred()
{
    ci.GraphicsPipeline.NumRenderTargets = 2;
    ci.PSODesc.PipelineType = PIPELINE_TYPE_GRAPHICS;
    ci.GraphicsPipeline.RTVFormats[0] = DIFFUSE_FORMAT;
    ci.GraphicsPipeline.RTVFormats[1] = NORMAL_FORMAT;
    ci.GraphicsPipeline.DSVFormat = DEPTH_FORMAT;
    ci.GraphicsPipeline.PrimitiveTopology = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    ci.GraphicsPipeline.RasterizerDesc.CullMode = CULL_MODE_BACK;
    ci.GraphicsPipeline.DepthStencilDesc.DepthEnable = true;
    ci.GraphicsPipeline.DepthStencilDesc.DepthWriteEnable = true;
    ci.PSODesc.ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE;
    return *this;
}

Hym::Pipeline& Hym::Pipeline::SetDefaultComposite()
{
    ci.PSODesc.Name = "Composite PSO";
    ci.PSODesc.PipelineType = PIPELINE_TYPE_GRAPHICS;
    ci.PSODesc.ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE;
    ci.GraphicsPipeline.PrimitiveTopology = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    ci.GraphicsPipeline.NumRenderTargets = 1;
    ci.GraphicsPipeline.RTVFormats[0] = SwapChain->GetDesc().ColorBufferFormat;
    ci.GraphicsPipeline.DepthStencilDesc.DepthEnable = false;
    ci.GraphicsPipeline.DepthStencilDesc.DepthWriteEnable = false;
    ci.GraphicsPipeline.RasterizerDesc.CullMode = CULL_MODE_NONE;
    return *this;
}

Hym::Pipeline& Hym::Pipeline::SetDefaultForward()
{
    ci.GraphicsPipeline.NumRenderTargets = 1;
    ci.GraphicsPipeline.RTVFormats[0] = SwapChain->GetDesc().ColorBufferFormat;
    ci.GraphicsPipeline.DSVFormat = DEPTH_FORMAT;// SwapChain->GetDesc().DepthBufferFormat;
    ci.GraphicsPipeline.PrimitiveTopology = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    ci.PSODesc.ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE;
    ci.GraphicsPipeline.RasterizerDesc.CullMode = CULL_MODE_BACK;
    ci.GraphicsPipeline.DepthStencilDesc.DepthEnable = True;
    return *this;
}

//Hym::Pipeline& Hym::Pipeline::SetDefaultRenderToTexture(TextureDesc* desc, int count)
//{
//    ci.PSODesc.Name = "Default Render To Texture PSO";
//    ci.PSODesc.PipelineType = PIPELINE_TYPE_GRAPHICS;
//    ci.PSODesc.ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE;
//    ci.GraphicsPipeline.NumRenderTargets = count;
//    for (int i = 0; i < count; i++)
//    {
//        ci.GraphicsPipeline.RTVFormats[i] = desc[i].Format;
//    }
//    ci.GraphicsPipeline.DepthStencilDesc.DepthEnable = false;
//    ci.GraphicsPipeline.DepthStencilDesc.DepthWriteEnable = false;
//    ci.GraphicsPipeline.RasterizerDesc.CullMode = CULL_MODE_NONE;
//    return *this;
//}

Hym::Pipeline& Hym::Pipeline::SetName(const char* name)
{
    ci.PSODesc.Name = name;
    return *this;
}

void Hym::Pipeline::Create()
{
    Dev->CreateGraphicsPipelineState(ci, &pso);
}

void Hym::Pipeline::CreateSRB()
{
    pso->CreateShaderResourceBinding(&srb);
}

Hym::ComputePipeline::ComputePipeline()
{
}

Hym::ComputePipeline& Hym::ComputePipeline::SetDefault()
{
    ci.PSODesc.PipelineType = PIPELINE_TYPE_COMPUTE;
    ci.PSODesc.ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE;
    return *this;
}


Hym::ComputePipeline& Hym::ComputePipeline::SetCS(const char* name, const char* filename, const ArrayRef<std::pair<const char*, const char*>>& macros, const char* entry)
{
    ShaderCreateInfo ShaderCI;

    ShaderMacroHelper helper;
    
    for (int i = 0; i<macros.size; i++)
    {
        auto& macro = macros.data[i];
        helper.AddShaderMacro(macro.first, macro.second);
    }

    helper.Finalize();

    ShaderCI.SourceLanguage = SHADER_SOURCE_LANGUAGE_HLSL;
    //ShaderCI.UseCombinedTextureSamplers = true;
    ShaderCI.ShaderCompiler = SHADER_COMPILER_DXC;
    ShaderCI.HLSLVersion = { 6, 5 };
    if(macros.size != 0)
        ShaderCI.Macros = helper;

    ShaderCI.pShaderSourceStreamFactory = ShaderStream;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_COMPUTE;
        ShaderCI.EntryPoint = entry;
        ShaderCI.Desc.Name = name;
        ShaderCI.FilePath = filename;
        Dev->CreateShader(ShaderCI, &cs);
    }
    ci.pCS = cs;
    return *this;
}

Hym::ComputePipeline& Hym::ComputePipeline::SetName(const char* name)
{
    ci.PSODesc.Name = name;
    return *this;
}

Hym::ComputePipeline& Hym::ComputePipeline::SetSamplers(const ArrayRef<ImmutableSamplerDesc>& a)
{
    this->ci.PSODesc.ResourceLayout.ImmutableSamplers = a.data;
    this->ci.PSODesc.ResourceLayout.NumImmutableSamplers = a.size;
    return *this;
}

void Hym::ComputePipeline::Create()
{
    Dev->CreateComputePipelineState(ci, &pso);
}

void Hym::ComputePipeline::CreateSRB()
{
    pso->CreateShaderResourceBinding(&srb);
}

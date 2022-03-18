#pragma once

#include "Common/interface/RefCntAutoPtr.hpp"
#include "RenderDevice.h"
#include "DeviceContext.h"
#include "Texture.h"
#include <vector>
#include "Definitions.h"

#define DIFFUSE_FORMAT dl::TEX_FORMAT_RGBA8_UNORM;
#define NORMAL_FORMAT dl::TEX_FORMAT_RGBA16_FLOAT;
#define DEPTH_FORMAT dl::TEX_FORMAT_D32_FLOAT;

// Reference: https://github.com/DiligentGraphics/DiligentSamples/blob/master/Tutorials/Tutorial19_RenderPasses/src/Tutorial19_RenderPasses.cpp
namespace Hym
{
	using namespace Diligent;
	class Pipeline
	{
	public:
		Pipeline();
		Pipeline& SetCullmode(CULL_MODE cm);
		Pipeline& SetDepthTesting(bool enable);
		Pipeline& SetLayoutElems(const ArrayRef<LayoutElement>& a);
		Pipeline& SetVS(const char* name, const char* filename);
		Pipeline& SetPS(const char* name, const char* filename);
		Pipeline& SetShaderVars(const ArrayRef<ShaderResourceVariableDesc>& a);
		GraphicsPipelineStateCreateInfo& GetPSOCi();
		//Pipeline& setDefault();
		Pipeline& SetDefaultDeferred();
		Pipeline& SetDefaultComposite();
		//Pipeline& SetDefaultRenderToTexture(TextureDesc* desc, int count);
		Pipeline& SetName(const char* name);
		void Create();
		void CreateSRB();
		IPipelineState& GetPSO() { return *pso; }
	private:
		RefCntAutoPtr<IPipelineState> pso;
		RefCntAutoPtr<IShaderResourceBinding> srb;
		GraphicsPipelineStateCreateInfo ci;

		RefCntAutoPtr<IShader> ps;
		RefCntAutoPtr<IShader> vs;
	};

	class ComputePipeline
	{
	public:
		ComputePipeline();
		ComputePipeline& SetDefault();
		ComputePipeline& SetCS(const char* name, const char* filename, const std::vector<std::pair<const char*, const char*>>& macros = {}, const char* entry = "main");
		ComputePipeline& SetName(const char* name);
		ComputePipeline& SetSamplers(const ArrayRef<ImmutableSamplerDesc>& a);
		void Create();
		void CreateSRB();
		IShaderResourceBinding* GetSRB() { return srb.RawPtr(); }
		IPipelineState* GetPSO() { return pso; }
	private:
		RefCntAutoPtr<IShader> cs;
		RefCntAutoPtr<IPipelineState> pso;
		RefCntAutoPtr<IShaderResourceBinding> srb;
		ComputePipelineStateCreateInfo ci;
	};

}


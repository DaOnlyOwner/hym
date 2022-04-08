#include "IrradianceField.h"
#include "Renderer.h"
#include <cassert>
#include "fmt/format.h"
#include <string>
#include "glm/gtx/quaternion.hpp"
#include "MapHelper.hpp"
#include "Debug.h"

#define WEIGHT_FORMAT dl::TEX_FORMAT_RGBA16_FLOAT;
#define IRRAD_FORMAT dl::TEX_FORMAT_RGBA16_FLOAT;

void Hym::IrradianceField::Init(int probesX, int probesY, int probesZ, int raysPerProbe, Scene& scene)
{
	auto& device = Dev;
	auto [minScene, maxScene] = scene.GetMinMax();
	L.probeCounts.x = probesX;
	L.probeCounts.y = probesY;
	L.probeCounts.z = probesZ;
	L.depthProbeSideLength = 16;
	L.irradianceProbeSideLength = 8;
	L.normalBias = 0.25f;
	L.irradianceTextureWidth = (L.irradianceProbeSideLength + 2) /* 1px Border around probe left and right */ * L.probeCounts.x * L.probeCounts.y + 2 /* 1px Border around whole texture left and right*/;
	L.irradianceTextureHeight = (L.irradianceProbeSideLength + 2) * L.probeCounts.z + 2;
	L.depthTextureWidth = (L.depthProbeSideLength + 2) * L.probeCounts.x * L.probeCounts.y + 2;
	L.depthTextureHeight = (L.depthProbeSideLength + 2) * L.probeCounts.z + 2;
	L.probeStartPosition = minScene;
	L.probeStep = glm::max((maxScene - minScene) / (glm::vec3(L.probeCounts) - glm::vec3(1)), glm::vec3(1, 1, 1));
	L.raysPerProbe = raysPerProbe;

	updateValues.depthSharpness = 50.0f;
	updateValues.hysteresis = 0.95f;
	auto probeEnd = L.probeStartPosition + glm::vec3((L.probeCounts - 1)) * L.probeStep;
	auto probeSpan = probeEnd - L.probeStartPosition;

	updateValues.maxDistance = glm::length(probeSpan / glm::vec3(L.probeCounts)) * 1.5f;

	TextureDesc desc;
	desc.Name = "Irradiance Texture";
	desc.Type = RESOURCE_DIM_TEX_2D;
	desc.BindFlags = BIND_UNORDERED_ACCESS | BIND_SHADER_RESOURCE;
	desc.Format = IRRAD_FORMAT;
	desc.Width = L.irradianceTextureWidth;
	desc.Height = L.irradianceTextureHeight;
	device->CreateTexture(desc, nullptr, &irradianceTex);

	desc.Name = "Weight Texture";
	desc.Format = WEIGHT_FORMAT;
	desc.Width = L.depthTextureWidth;
	desc.Height = L.depthTextureHeight;
	desc.BindFlags = BIND_UNORDERED_ACCESS | BIND_SHADER_RESOURCE;
	device->CreateTexture(desc, nullptr, &weightTex);


	surfelWidth = L.raysPerProbe;
	surfelHeight = L.probeCounts.x * L.probeCounts.y * L.probeCounts.z;
	createSurfelBuffer(rayHitLocations, "rayHitLocations");
	createSurfelBuffer(rayDirections, "rayDirections");
	createSurfelBuffer(rayOrigins, "rayOrigins");
	createSurfelBuffer(rayHitNormals, "rayHitNormals");
	createSurfelBuffer(rayHitRadiance, "rayHitRadiance");

	SamplerDesc sdesc
	{
		FILTER_TYPE_LINEAR,FILTER_TYPE_LINEAR,FILTER_TYPE_LINEAR,
		TEXTURE_ADDRESS_CLAMP,TEXTURE_ADDRESS_CLAMP,TEXTURE_ADDRESS_CLAMP
	};
	
	SamplerDesc sdescAniso{
		FILTER_TYPE_ANISOTROPIC, FILTER_TYPE_ANISOTROPIC, FILTER_TYPE_ANISOTROPIC,
		TEXTURE_ADDRESS_WRAP, TEXTURE_ADDRESS_WRAP, TEXTURE_ADDRESS_WRAP, 0.f, 8 //
	};

	ImmutableSamplerDesc samplers[] =
	{
		{SHADER_TYPE_COMPUTE, "irradianceTex_sampler",sdesc},
		{SHADER_TYPE_COMPUTE, "weightTex_sampler",sdesc},
		{SHADER_TYPE_COMPUTE, "tex_sampler",sdescAniso}
	};

	std::string albedoTexsSize = std::to_string(scene.GetResourceManager().GetAlbedoTexs().size());
	std::pair<const char*, const char*> macros[] =
	{
		{"NUM_ALBEDO_TEXS", albedoTexsSize.c_str()}
	};

	computeRaysPass
		.SetDefault()
		.SetCS("Compute Rays CS", SHADER_RES "/computeRays_cs.hlsl", ArrayRef<std::pair<const char*, const char*>>::MakeRef(macros, _countof(macros)))
		.SetName("Compute Rays PSO")
		.SetSamplers(ArrayRef<ImmutableSamplerDesc>::MakeRef(samplers, _countof(samplers)))
		.Create();
	
	lightFieldBuffer = UniformBuffer<LightField>("Lightfield Buffer", USAGE_IMMUTABLE, L);
	updateValuesBuffer = UniformBuffer<UpdateValues>("Update Values Buffer", USAGE_IMMUTABLE, updateValues);
	randomOrientationBuffer = UniformBuffer<RandomOrientation>("Random Orientation Buffer");
	
	computeRaysPass.CreateSRB();
	auto* srb = computeRaysPass.GetSRB();
	srb->GetVariableByName(SHADER_TYPE_COMPUTE, "tlas")->Set(scene.GetTLAS());
	// For StructuredBuffer always GetDefaultView(), for ConstantBuffer just the buffer obj.
	srb->GetVariableByName(SHADER_TYPE_COMPUTE, "vertexBuffer")->Set(scene.GetResourceManager().GetVertexBuffer()->GetDefaultView(BUFFER_VIEW_SHADER_RESOURCE));
	srb->GetVariableByName(SHADER_TYPE_COMPUTE, "indexBuffer")->Set(scene.GetResourceManager().GetIndexBuffer()->GetDefaultView(BUFFER_VIEW_SHADER_RESOURCE));
	srb->GetVariableByName(SHADER_TYPE_COMPUTE, "irradianceTex")->Set(irradianceTex->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE));
	srb->GetVariableByName(SHADER_TYPE_COMPUTE, "weightTex")->Set(weightTex->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE));
	srb->GetVariableByName(SHADER_TYPE_COMPUTE, "L")->Set(lightFieldBuffer.GetBuffer());
	srb->GetVariableByName(SHADER_TYPE_COMPUTE, "sun")->Set(scene.GetSun().GetSunBuffer());
	srb->GetVariableByName(SHADER_TYPE_COMPUTE, "randomOrientation")->Set(randomOrientationBuffer.GetBuffer());
	srb->GetVariableByName(SHADER_TYPE_COMPUTE, "rayHitLocations")->Set(rayHitLocations->GetDefaultView(TEXTURE_VIEW_UNORDERED_ACCESS));
	srb->GetVariableByName(SHADER_TYPE_COMPUTE, "rayHitNormals")->Set(rayHitNormals->GetDefaultView(TEXTURE_VIEW_UNORDERED_ACCESS));
	srb->GetVariableByName(SHADER_TYPE_COMPUTE, "rayHitRadiance")->Set(rayHitRadiance->GetDefaultView(TEXTURE_VIEW_UNORDERED_ACCESS));
	srb->GetVariableByName(SHADER_TYPE_COMPUTE, "rayOrigins")->Set(rayOrigins->GetDefaultView(TEXTURE_VIEW_UNORDERED_ACCESS));
	srb->GetVariableByName(SHADER_TYPE_COMPUTE, "rayDirections")->Set(rayDirections->GetDefaultView(TEXTURE_VIEW_UNORDERED_ACCESS));
	srb->GetVariableByName(SHADER_TYPE_COMPUTE, "attrs")->Set(scene.GetInstanceObjAttrsBuffer()->GetDefaultView(BUFFER_VIEW_SHADER_RESOURCE));
	srb->GetVariableByName(SHADER_TYPE_COMPUTE, "materials")->Set(scene.GetResourceManager().GetMaterialBuffer()->GetDefaultView(BUFFER_VIEW_SHADER_RESOURCE));
	auto views = scene.GetResourceManager().GetAlbedoTexViews();
	srb->GetVariableByName(SHADER_TYPE_COMPUTE, "albedoTexs")->SetArray(views.data(), 0,views.size());

	createPSOBorderOp(copyBorder_IrradiancePass, irradianceTex, "Copy Border Irradiance PSO", SHADER_RES "/borderOperations_cs.hlsl", "8", "mainDuplicateProbeEdges");
	createPSOBorderOp(onesBorder_IrradiancePass, irradianceTex, "Ones To Border Irradiance PSO", SHADER_RES "/borderOperations_cs.hlsl", "8", "mainWriteOnesToBorder");
	createPSOBorderOp(copyBorder_WeightPass, weightTex, "Copy Border Weight PSO", SHADER_RES "/borderOperations_cs.hlsl", "16", "mainDuplicateProbeEdges");
	createPSOBorderOp(onesBorder_WeightPass, weightTex, "Ones To Border Weight PSO", SHADER_RES "/borderOperations_cs.hlsl", "16", "mainWriteOnesToBorder");

	createUpdateIrrProbesPSO(updateIrradianceProbesPass, SHADER_RES "/updateIrradianceProbes_cs.hlsl", "Update Irradiance Probes CS", "Update Irradiance Probes PSO", "8");
	createUpdateIrrProbesPSO(updateWeightProbesPass, SHADER_RES "/updateIrradianceProbes_cs.hlsl", "Update Weight Probes CS", "Update Weight Probes PSO", "16");

}

void Hym::IrradianceField::createUpdateIrrProbesPSO(ComputePipeline& p, const char* filename, const char* name, const char* psoName, const char* probeSideLength)
{
	auto rpp = std::to_string(L.raysPerProbe);


	std::string psl(probeSideLength);
	std::vector<std::pair<const char*, const char*>> macros;
	macros.reserve(3);
	macros.push_back({ "PROBE_SIDE_LENGTH",probeSideLength });
	macros.push_back({ "RAYS_PER_PROBE", rpp.c_str() });
	if (psl == "8") // Irradiance texture
		macros.push_back({ "OUTPUT_IRRADIANCE","1" }); 
	p
		.SetDefault()
		.SetName(psoName)
		.SetCS(name, filename, ArrayRef<std::pair<const char*,const char*>>::MakeRef(macros))
		.Create();

	p.CreateSRB();
	auto srbUpdate = p.GetSRB();
	
	srbUpdate->GetVariableByName(SHADER_TYPE_COMPUTE, "rayDirections")->Set(rayDirections->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE));
	srbUpdate->GetVariableByName(SHADER_TYPE_COMPUTE, "rayHitRadiance")->Set(rayHitRadiance->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE));
	srbUpdate->GetVariableByName(SHADER_TYPE_COMPUTE, "rayHitNormals")->Set(rayHitNormals->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE));
	srbUpdate->GetVariableByName(SHADER_TYPE_COMPUTE, "rayHitLocations")->Set(rayHitLocations->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE));
	srbUpdate->GetVariableByName(SHADER_TYPE_COMPUTE, "rayOrigins")->Set(rayOrigins->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE));
	srbUpdate->GetVariableByName(SHADER_TYPE_COMPUTE, "uniforms")->Set(updateValuesBuffer.GetBuffer());
	srbUpdate->GetVariableByName(SHADER_TYPE_COMPUTE, "tex")->Set(psl == "8" ? irradianceTex->GetDefaultView(TEXTURE_VIEW_UNORDERED_ACCESS)
		: weightTex->GetDefaultView(TEXTURE_VIEW_UNORDERED_ACCESS));

	std::random_device dev;
	rd = std::mt19937_64(dev());
	distr = std::uniform_real_distribution<double>(0., glm::pi<double>() * 2);
	distrAxis = std::uniform_real_distribution<double>(0., 10000000.);
}

void Hym::IrradianceField::Draw()
{
	auto tgcXIrr = std::ceil(L.irradianceTextureWidth / (float)numThreadsX);
	auto tgcYIrr = std::ceil(L.irradianceTextureHeight / (float)numThreadsY);
	auto tgcXWei = std::ceil(L.depthTextureWidth / (float)numThreadsX);
	auto tgcYWei = std::ceil(L.depthTextureHeight / (float)numThreadsY);

	DispatchComputeAttribs dattrs;
	dattrs.ThreadGroupCountZ = 1;
	// https://stackoverflow.com/questions/8412630/how-to-execute-a-piece-of-code-only-once
	static bool once = [&]() {

		dattrs.ThreadGroupCountX = tgcXIrr;
		dattrs.ThreadGroupCountY = tgcYIrr;
		Imm->SetPipelineState(onesBorder_IrradiancePass.GetPSO());
		Imm->CommitShaderResources(onesBorder_IrradiancePass.GetSRB(), RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
		Imm->DispatchCompute(dattrs);

		dattrs.ThreadGroupCountX = tgcXWei;
		dattrs.ThreadGroupCountY = tgcYWei;
		Imm->SetPipelineState(onesBorder_WeightPass.GetPSO());
		Imm->CommitShaderResources(onesBorder_WeightPass.GetSRB(), RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
		Imm->DispatchCompute(dattrs);

		return true;
	}();

	auto tgcXSurfel = std::ceil(surfelWidth / (float)numThreadsX);
	auto tgcYSurfel = std::ceil(surfelHeight / (float)numThreadsY);
	dattrs.ThreadGroupCountX = tgcXSurfel;
	dattrs.ThreadGroupCountY = tgcYSurfel;
	Imm->SetPipelineState(computeRaysPass.GetPSO());
	Imm->CommitShaderResources(computeRaysPass.GetSRB(), RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

	{
		auto map = randomOrientationBuffer.Map();
		auto t1 = distrAxis(rd);
		auto t2 = distrAxis(rd);
		auto t3 = distrAxis(rd);
		glm::vec3 axis(t1, t2, t3);
		float angle = distr(rd);
		map->mat = glm::mat3(glm::angleAxis(angle, axis));
	}

	Imm->DispatchCompute(dattrs);

	dattrs.ThreadGroupCountX = tgcXWei;
	dattrs.ThreadGroupCountY = tgcYWei;
	
	Imm->SetPipelineState(updateWeightProbesPass.GetPSO());
	Imm->CommitShaderResources(updateWeightProbesPass.GetSRB(), RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
	Imm->DispatchCompute(dattrs);

	Imm->SetPipelineState(copyBorder_WeightPass.GetPSO());
	Imm->CommitShaderResources(copyBorder_WeightPass.GetSRB(), RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
	Imm->DispatchCompute(dattrs);

	dattrs.ThreadGroupCountX = tgcXIrr;
	dattrs.ThreadGroupCountY = tgcXIrr;

	Imm->SetPipelineState(updateIrradianceProbesPass.GetPSO());
	Imm->CommitShaderResources(updateIrradianceProbesPass.GetSRB(), RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
	Imm->DispatchCompute(dattrs);

	Imm->SetPipelineState(copyBorder_IrradiancePass.GetPSO());
	Imm->CommitShaderResources(copyBorder_IrradiancePass.GetSRB(), RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
	Imm->DispatchCompute(dattrs);

}

void Hym::IrradianceField::createSurfelBuffer(RefCntAutoPtr<ITexture>& buffer, const char* name)
{
	TextureDesc desc;
	desc.Name = name;
	desc.Type = RESOURCE_DIM_TEX_2D;
	desc.BindFlags = BIND_UNORDERED_ACCESS | BIND_SHADER_RESOURCE;
	desc.Format = TEX_FORMAT_RGBA32_FLOAT;
	desc.Width = L.raysPerProbe;
	desc.Height = L.probeCounts.x * L.probeCounts.y * L.probeCounts.z;
	Dev->CreateTexture(desc, nullptr, &buffer);
}

void Hym::IrradianceField::createPSOBorderOp(ComputePipeline& p, ITexture* tex, const char* name, const char* filename, const char* probeSideLength, const char* entry)
{
	auto fmtted = fmt::format("{} CS", name);
	std::pair<const char*, const char*> macros[]
		= { {"PROBE_SIDE_LENGTH",probeSideLength} };
	auto ref = ArrayRef<std::pair<const char*, const char*>>::MakeRef(macros, _countof(macros));
	p.SetDefault()
		.SetName(name)
		.SetCS(fmtted.c_str(), filename, ref, entry)
		.Create();

	p.CreateSRB();
	auto srb = p.GetSRB();
	srb->GetVariableByName(SHADER_TYPE_COMPUTE, "tex")->Set(tex->GetDefaultView(TEXTURE_VIEW_UNORDERED_ACCESS));
}

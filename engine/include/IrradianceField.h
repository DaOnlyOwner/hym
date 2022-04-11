#pragma once
#include "GPUBuffer.h"
#include "glm/vec3.hpp"
#include "Common/interface/RefCntAutoPtr.hpp"
#include "Texture.h"
#include "Pipeline.h"
#include "ShaderResourceBinding.h"
#include "Sun.h"
#include "glm/mat3x3.hpp"
#include <random>
#include "HymScene.h"


namespace Hym
{
	using namespace Diligent;

	struct LightField
	{
		glm::ivec3 probeCounts;
		int raysPerProbe;
		glm::vec3 probeStartPosition;
		float normalBias;
		glm::vec3 probeStep;
		int irradianceTextureWidth;
		int irradianceTextureHeight;
		float irradianceProbeSideLength;
		int depthTextureWidth;
		int depthTextureHeight;
		float depthProbeSideLength;
		float padding1, padding2, padding3;
	};


	struct UpdateValues
	{
		float depthSharpness;
		float hysteresis;
		float maxDistance;
	};


	struct RandomOrientation
	{
		glm::mat4 mat;
	};

	class IrradianceField
	{
	public:
		void Init(int probesX, int probesY, int probesZ, int raysPerProbe, Scene& scene);
		void Draw();
		//void DebugDrawProbes();

		dl::ITextureView* GetIrrTexView() { return irradianceTex->GetDefaultView(dl::TEXTURE_VIEW_SHADER_RESOURCE); }
		dl::ITexture* GetIrrTex() { return irradianceTex.RawPtr(); }
		dl::ITextureView* GetWeightTexView() { return weightTex->GetDefaultView(dl::TEXTURE_VIEW_SHADER_RESOURCE); }
		dl::ITextureView* GetRayHitLocationsView() { return rayHitLocations->GetDefaultView(dl::TEXTURE_VIEW_SHADER_RESOURCE); }
		dl::ITextureView* GetRayHitRadianceView() { return rayHitRadiance->GetDefaultView(dl::TEXTURE_VIEW_SHADER_RESOURCE); }
		dl::ITextureView* GetRayOriginsView() { return rayOrigins->GetDefaultView(dl::TEXTURE_VIEW_SHADER_RESOURCE); }
		dl::ITextureView* GetRayHitNormalsView() { return rayHitNormals->GetDefaultView(dl::TEXTURE_VIEW_SHADER_RESOURCE); }
		dl::ITexture* GetWeightTex() { return weightTex.RawPtr(); }
		dl::IBuffer* GetLightFieldBuffer() { return lightFieldBuffer.GetBuffer(); }
		const LightField& GetLightField() { return L; }

	private:
		glm::vec3 pos;
		LightField L;
		UpdateValues updateValues;
		int surfelWidth;
		int surfelHeight;
		static constexpr int probeLengthWeight = 16;
		static constexpr int probeLengthIrradiance = 8;
		static constexpr int numThreadsX = 8;
		static constexpr int numThreadsY = 8;
		
		bool writeToOnesDone = false;

		std::mt19937_64 rd;
		std::uniform_real_distribution<double> distr;
		std::uniform_real_distribution<double> distrAxis;

		RefCntAutoPtr<ITexture> weightTex;
		RefCntAutoPtr<ITexture> irradianceTex;

		RefCntAutoPtr<ITexture> rayHitLocations;
		RefCntAutoPtr<ITexture> rayHitRadiance;
		RefCntAutoPtr<ITexture> rayHitNormals;
		RefCntAutoPtr<ITexture> rayDirections;
		RefCntAutoPtr<ITexture> rayOrigins;

		ComputePipeline computeRaysPass;

		ComputePipeline copyBorder_WeightPass;
		ComputePipeline onesBorder_WeightPass;
		ComputePipeline copyBorder_IrradiancePass;
		ComputePipeline onesBorder_IrradiancePass;

		ComputePipeline updateIrradianceProbesPass;
		ComputePipeline updateWeightProbesPass;

		UniformBuffer<LightField> lightFieldBuffer;
		UniformBuffer<UpdateValues> updateValuesBuffer;
		UniformBuffer<RandomOrientation> randomOrientationBuffer;

		void createSurfelBuffer(RefCntAutoPtr<ITexture>& buffer, const char* name);
		void createPSOBorderOp(ComputePipeline& p, ITexture* tex, const char* name, const char* filename, const char* probeSideLength, const char* entry);
		void createUpdateIrrProbesPSO(ComputePipeline& p, const char* filename, const char* name, const char* psoName, const char* probeSideLength, bool outputIrr);


	};
}
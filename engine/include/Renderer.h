#pragma once
#include "Definitions.h"
#include "Texture.h"
#include "glm/mat4x4.hpp"
#include "HymScene.h"
#include "GenericCamera.h"
#include "Pipeline.h"
#include "GPUBuffer.h"
#include "IrradianceField.h"

namespace Hym
{

	struct GeometryPassVSPerMeshConstants
	{
		glm::mat4 model; // 4x4 * sizeof(float) = 16 * 4
		glm::mat4 normal; // 4 * 4 = 16 * 4
		glm::mat4 MVP; // 4 * 4  = 16 * 4  
		u32 matIdx;
		float p1, p2, p3;
	};

	struct View
	{
		glm::mat4 VPInv;
		glm::vec3 eyePos;
		int showHitLocations;
		int probeID;
		int p1, p2, p3;
	};


	class Renderer
	{
	public:
		Renderer() = default;
		void Init(Scene& scene, int probesX, int probesY, int probesZ, int raysPerProbe);
		void RebuildSRBs(class Scene& resource);
		//void InitIrrField(int pX, int pY, int pZ, int raysPerProbe, const glm::vec3& minScene, const glm::vec3& maxScene);
		//void InitTLAS();
		//~Renderer() {}

		void Draw(Scene& scene, Camera& cam);
		void Resize(Scene& scene);
		void DoDebugDrawProbes(bool enable) { debugRenderProbes = enable; }
		void DoShowHitLocations(bool enable) { showHitLocations = enable; }
		void SetDebugProbeID(int id) { currentDebugProbeID = id; }
		//void SetSun(const Sun& sun);

	private:

		struct DebugDrawProbesData
		{
			struct Uniforms
			{
				int probeID;
				int sideLength;
				int debugProbeID, padding3;
			};
			Pipeline pso;
			Mesh mesh;
			UniformBuffer<Uniforms> uniforms;
		};

		DebugDrawProbesData debugProbeData;


		struct GBuffer
		{
			RefCntAutoPtr<dl::ITexture> albedoBuffer;
			RefCntAutoPtr<dl::ITexture> normalBuffer;
			RefCntAutoPtr<dl::ITexture> depthBuffer;
		};

		Pipeline geometryPass;
		RefCntAutoPtr<IShaderResourceBinding> geomSRB;
		RefCntAutoPtr<ISampler> anisoSampler;
		UniformBuffer<GeometryPassVSPerMeshConstants> geomConstants;

		Pipeline compositePass;
		RefCntAutoPtr<IShaderResourceBinding> compositeSRB;
		//RefCntAutoPtr<IBuffer> viewBuffer;
		UniformBuffer<View> viewBuffer;

		IrradianceField irrField;

		GBuffer gbuffer;
		//IrradianceField irrField;
		//RefCntAutoPtr<IBuffer> sunBuffer;
		dl::SamplerDesc createAnisoSampler();
		dl::SamplerDesc createLinearSampler();

		void initGeometryPSO(Scene& scene);
		void initGeometrySRB(Scene& scene);
		void initGeometryBuffers();

		void initCompositePSO(Scene& scene);
		//void initCompositeBuffers();
		void initCompositeSRB(Scene& scene);

		void initRenderTextures();

		bool debugRenderProbes = false;
		bool showHitLocations = false;
		int currentDebugProbeID = 0;
		void debugDrawProbes(Scene& scene, Camera& cam);

		void initDebugDrawProbes(Hym::Scene& scene);

	};
}
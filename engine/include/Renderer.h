#pragma once
#include "Definitions.h"
#include "Texture.h"
#include "glm/mat4x4.hpp"
#include "HymScene.h"
#include "GenericCamera.h"
#include "Pipeline.h"
#include "GPUBuffer.h"

namespace Hym
{

	struct GeometryPassVSPerMeshConstants
	{
		glm::mat4 model; // 4x4 * sizeof(float) = 16 * 4
		glm::mat4 normal; // 4 * 4 = 16 * 4
		glm::mat4 MVP; // 4 * 4  = 16 * 4  
		// => 16 * 64 => Multiple of 16
	};

	struct View
	{
		glm::mat4 VPInv;
		glm::vec3 eyePos;
		float padding;
	};


	class Renderer
	{
	public:
		Renderer() = default;
		void Init();
		//void InitIrrField(int pX, int pY, int pZ, int raysPerProbe, const glm::vec3& minScene, const glm::vec3& maxScene);
		//void InitTLAS();
		//~Renderer() {}

		void Draw(Scene& scene, Camera& cam);
		void EndFrame();
		void Resize();
		//void SetSun(const Sun& sun);

	private:

		struct GBuffer
		{
			RefCntAutoPtr<dl::ITexture> albedoBuffer;
			RefCntAutoPtr<dl::ITexture> normalBuffer;
			RefCntAutoPtr<dl::ITexture> depthBuffer;
		};

		Pipeline geometryPass;
		RefCntAutoPtr<IShaderResourceBinding> geomSRB;
		UniformBuffer<GeometryPassVSPerMeshConstants> geomConstants;

		Pipeline compositePass;
		RefCntAutoPtr<IShaderResourceBinding> compositeSRB;
		//RefCntAutoPtr<IBuffer> viewBuffer;
		UniformBuffer<View> viewBuffer;


		GBuffer gbuffer;
		//IrradianceField irrField;
		//RefCntAutoPtr<IBuffer> sunBuffer;

		void initGeometryPSO();
		void initGeometrySRB();
		void initGeometryBuffers();

		void initCompositePSO();
		//void initCompositeBuffers();
		void initCompositeSRB();

		void initRenderTextures();

	};
}
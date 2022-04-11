#pragma once
#include <string>
#include "Components.h"
#include <vector>
#include <unordered_map>
#include <map>
#include "GPUBuffer.h"
#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"
#include "GPUTexture.h"
#include <filesystem>


namespace Hym
{

	class ResourceManager
	{
	public:
		typedef std::vector<RefCntAutoPtr<dl::ITexture>> TexArray;
		//typedef std::vector<dl::ITextureView> TexView;
		Texture LoadTexture(const std::string& filename, const std::string& alias, TextureType type);
		void LoadSceneFile(const std::string& filename, const std::string& alias);
		const ModelComponent* GetModel(const std::string& sceneAlias, const std::string& objName) const;
		const Texture* GetTexture(const std::string& name);
		//bool CreateNewModel(const std::string& objName, const ModelComponent& mc) const; 
		const std::vector<ModelComponent>* GetSceneModels(const std::string& sceneAlias) const;
		//u32 CreateMaterial(const Material& material);
		//ModelComponent* CreateModel(Mesh)
		StructuredBuffer<Vertex>& GetVertexBuffer() { return globalVertexBuffer; }
		StructuredBuffer<u32>& GetIndexBuffer() { return globalIndexBuffer; }
		StructuredBuffer<Material>& GetMaterialBuffer() { return globalMaterialBuffer; }
		Texture GetTexture(u64 id, TextureType ttype);
		std::vector<dl::IDeviceObject*> GetAlbedoTexViews() { return createViews(albedoTexs); }
		TexArray& GetAlbedoTexs() { return albedoTexs; }
		TexArray& GetNormalTexs() { return normalTexs; } // TODO: Views
		TexArray& GetMetalRoughnessTexs() { return metalRoughnessTexs; }
		TexArray& GetEmissiveTexs() { return emissiveTexs; }
		dl::IBottomLevelAS* GetBlas(u64 blasIdx) { return blasVec[blasIdx]; } 
		const std::string& GetName(u64 idx) { return meshNames[idx]; }
		const std::pair<glm::vec3, glm::vec3>& GetMinMax(u64 idx) { return minMaxVec[idx]; }
		//void CreateBLASForScene(const std::string& scene);
		void Init();
		void calcMinMax(const Hym::u64& globalVerticesAt, std::vector<Hym::Vertex>& vertices);
		
	private:
		void preTransformNode(const aiNode& node, const aiScene& scene, std::vector<ModelComponent>& models, std::vector<Vertex>& vertices, std::vector<u32>& indices, std::vector<Material>& materials);
		Mesh createMesh(u64 vAt, u64 iAt, u64 indexSize, u64 verticesSize, const char* name);
		u32 createMaterial(aiMaterial* ai_mat, const std::string& meshName, std::vector<Material>& materials);
		bool upload(const std::vector<Vertex>& vertices, const std::vector<u32>& indices, const std::vector<Material>& materials);
		void createBLAS(std::vector<ModelComponent>& models);
		std::vector<dl::IDeviceObject*> createViews(std::vector<RefCntAutoPtr<dl::ITexture>>& texs);
		//void createIdxIntoLinearBuffer(std::vector<ModelComponent>& models);

	private:

		std::filesystem::path currSceneFileDir;

		std::unordered_map<std::string, std::vector<ModelComponent>> aliasMap;
		StructuredBuffer<Vertex> globalVertexBuffer;
		StructuredBuffer<u32> globalIndexBuffer;
		StructuredBuffer<Material> globalMaterialBuffer;
		std::unordered_map<std::string, Texture> textureAliasMap;
		std::vector<RefCntAutoPtr<dl::IBottomLevelAS>> blasVec;
		std::vector<std::string> meshNames;
		std::vector<std::pair<glm::vec3, glm::vec3>> minMaxVec;
		
		RefCntAutoPtr<dl::IBuffer> blasScratchBuffer;

		TexArray albedoTexs;
		TexArray normalTexs;
		TexArray metalRoughnessTexs;
		TexArray emissiveTexs;

		u64 meshIdx = 0;


	};
}
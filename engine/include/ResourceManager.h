#pragma once
#include <string>
#include "Components.h"
#include <vector>
#include <unordered_map>
#include "GPUBuffer.h"
#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"
#include "GPUTexture.h"

namespace Hym
{

	class ResourceManager
	{
	public:
		typedef std::vector<Texture> TexArray;
		//typedef std::vector<dl::ITextureView> TexView;
		struct Texs
		{
			TexArray textures;
			//TexView views;
		};
		Texture LoadTexture(const std::string& filename, TextureType type);
		void LoadSceneFile(const std::string& filename, const std::string& alias);
		const ModelComponent* GetModel(const std::string& sceneAlias, const std::string& objName) const;
		const std::vector<ModelComponent>* GetSceneModels(const std::string& sceneAlias) const;
		StructuredBuffer<Vertex>& GetVertexBuffer() { return globalVertexBuffer; }
		StructuredBuffer<u32>& GetIndexBuffer() { return globalIndexBuffer; }
		Texture& GetTexture(u64 id, TextureType ttype);
		Texs& GetAlbedoTexs() { return albedoTexs; }
		Texs& GetNormalTexs() { return normalTexs; }
		Texs& GetMetalRoughnessTexs() { return metalRoughnessTexs; }
		Texs& GetEmissiveTexs() { return emissiveTexs; }
		void CreateBLASForScene(const std::string& scene);
		void Init();
		
	private:
		void preTransformNode(const aiNode& node, const aiScene& scene, std::vector<ModelComponent>& models, std::vector<Vertex>& vertices, std::vector<u32>& indices);
		Mesh createMesh(u64 vAt, u64 iAt, u64 indexSize, u64 verticesSize, const char* name);
		Material createMaterial(aiMaterial* ai_mat);
		void upload(std::vector<Vertex>& vertices, std::vector<u32>& indices);

	private:
		std::unordered_map<std::string, std::vector<ModelComponent>> aliasMap;
		StructuredBuffer<Vertex> globalVertexBuffer;
		StructuredBuffer<u32> globalIndexBuffer;
		Texs albedoTexs;
		Texs normalTexs;
		Texs metalRoughnessTexs;
		Texs emissiveTexs;
		RefCntAutoPtr<dl::IBuffer> blasScratchBuffer;


	};
}
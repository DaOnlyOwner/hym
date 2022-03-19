#include "ResourceManager.h"
#include "Debug.h"
#include "glm/mat4x4.hpp"
#include "fmt/format.h"
#include "TextureLoader.h"
#include "Align.hpp"
#include "GlobalState.h"

void createVertices(aiMesh* p_assMesh, const aiMatrix4x4& trans, std::vector<Hym::Vertex>& vertices)
{
	for (unsigned int i = 0; i < p_assMesh->mNumVertices; i++)
	{
		Hym::Vertex v;

		aiVector3D assPos = trans * p_assMesh->mVertices[i];

		if (!p_assMesh->HasTextureCoords(0))
		{
			HYM_ERROR("Imported mesh {} has no texture coordinates", std::string(p_assMesh->mName.C_Str()));
		}

		glm::vec3 pos{ assPos.x, assPos.y, assPos.z };
		v.pos = pos;

		auto normalMatrix = trans;
		normalMatrix.Inverse();
		normalMatrix.Transpose();
		aiVector3D assNormal = normalMatrix * p_assMesh->mNormals[i];
		aiVector3D assTangent = trans * p_assMesh->mTangents[i];
		aiVector3D assBitangent = trans * p_assMesh->mBitangents[i];
		glm::vec3 normal{ assNormal.x, assNormal.y, assNormal.z };
		glm::vec3 tangent{ assTangent.x, assTangent.y, assTangent.z };
		glm::vec3 bitangent{ assBitangent.x,assBitangent.y,assBitangent.z };


		v.normal = glm::normalize(normal);
		//v.tangent = glm::normalize(tangent);
		//v.bitangent = glm::normalize(bitangent);

		auto assUV = p_assMesh->mTextureCoords[0][i];
		glm::vec2 uvMap{ assUV.x,1 - assUV.y };
		v.uv = uvMap;

		vertices.push_back(v);
	}
	// Align for building BLAS
	auto alignment = Hym::Dev->GetAdapterInfo().RayTracing.VertexBufferAlignment;
	auto toAdd = Diligent::AlignUp(p_assMesh->mNumVertices * unsigned int(sizeof(Hym::Vertex)), alignment) / sizeof(Hym::Vertex) - p_assMesh->mNumVertices;
	for (int i = 0; i < toAdd; i++)
	{
		vertices.push_back({});
	}
}

void createIndices(aiMesh* p_assMesh, std::vector<Hym::u32>& indices)
{
	unsigned int count = 0;
	for (unsigned int i = 0; i < p_assMesh->mNumFaces; i++)
	{
		aiFace& face = p_assMesh->mFaces[i];

		for (unsigned int j = 0; j < face.mNumIndices; j++)
		{
			indices.push_back(face.mIndices[j]);
			count++;
		}
	}
	// Align for building BLAS
	auto alignment = Hym::Dev->GetAdapterInfo().RayTracing.IndexBufferAlignment;
	auto toAdd = Diligent::AlignUp(count * unsigned int(sizeof(Hym::u32)), alignment) / sizeof(Hym::u32) - count;
	for (int i = 0; i < toAdd; i++)
	{
		indices.push_back(0);
	}
}

Hym::Texture& Hym::ResourceManager::GetTexture(u64 id, TextureType ttype)
{
	switch (ttype)
	{
	case TextureType::Albedo:
		return albedoTexs.textures[id];
	case TextureType::Normal:
		return normalTexs.textures[id];
	case TextureType::MetalRough:
		return metalRoughnessTexs.textures[id];
	case TextureType::Emissive:
		return emissiveTexs.textures[id];
	default:
		HYM_WARN("Texture Type unknown");
		return albedoTexs.textures[0];
	}
}

void Hym::ResourceManager::CreateBLASForScene(const std::string& scene)
{
	auto it = aliasMap.find(scene);
	if (it != aliasMap.end())
	{
		auto& vec = it->second;
		for (auto& comp : vec)
		{
			
		}
	}
}

void Hym::ResourceManager::Init()
{
	HYM_WARN("TODO: Implement default texture");
	//globalVertexBuffer = StructuredBuffer<Vertex>("Global Vertex Buffer", dl::BIND_VERTEX_BUFFER | dl::BIND_RAY_TRACING | dl::BIND_SHADER_RESOURCE);
	//globalIndexBuffer = StructuredBuffer<u32>("Global Index Buffer", dl::BIND_INDEX_BUFFER | dl::BIND_RAY_TRACING | dl::BIND_SHADER_RESOURCE);
}

void Hym::ResourceManager::preTransformNode(const aiNode& node, const aiScene& scene, std::vector<Hym::ModelComponent>& models, std::vector<Vertex>& vertices, std::vector<u32>& indices)
{
	for (unsigned int i = 0; i < node.mNumChildren; i++)
	{
		aiNode& child = *node.mChildren[i];
		child.mTransformation = node.mTransformation * child.mTransformation;
		preTransformNode(child, scene, models,vertices,indices);
		for (unsigned int j = 0; j < child.mNumMeshes; j++)
		{
			auto* mesh = scene.mMeshes[child.mMeshes[j]];
			u64 globalVerticesAt = vertices.size();
			u64 globalIndicesAt = indices.size();
			createVertices(mesh, child.mTransformation,vertices);
			createIndices(mesh,indices);
			//globalVertexBuffer.Add(ArrayRef<Vertex>::MakeRef(vertices));
			//globalIndexBuffer.Add(ArrayRef<u32>::MakeRef(indices));
			auto indexSize = indices.size() - globalIndicesAt;
			auto verticesSize = vertices.size() - globalVerticesAt;
			auto mat = createMaterial(scene.mMaterials[mesh->mMaterialIndex]);
			auto mesh_ = createMesh(globalVerticesAt, globalIndicesAt, indexSize, verticesSize, mesh->mName.C_Str());
			ModelComponent modelComp;
			modelComp.mat = mat;
			modelComp.mesh = mesh_;
			models.push_back(modelComp);
		}
	}
}

Hym::Texture Hym::ResourceManager::LoadTexture(const std::string& filename, Hym::TextureType type)
{
	dl::TextureLoadInfo tlinfo;
	tlinfo.GenerateMips = true;
	tlinfo.IsSRGB = true;
	RefCntAutoPtr<dl::ITextureLoader> loader;
	RefCntAutoPtr<dl::ITexture> tex;
	dl::CreateTextureLoaderFromFile(filename.c_str(), dl::IMAGE_FILE_FORMAT_UNKNOWN, tlinfo, &loader);
	loader->CreateTexture(Dev, &tex);
	Texture out{ tex };
	switch (type)
	{
	case TextureType::Albedo:
		albedoTexs.textures.push_back(out);
		break;
	case TextureType::Normal:
		normalTexs.textures.push_back(out);
		break;
	case TextureType::MetalRough:
		metalRoughnessTexs.textures.push_back(out);
		break;
	case TextureType::Emissive:
		emissiveTexs.textures.push_back(out);
	}
	return out;
}

// TODO: Transform the errors into warnings when default texture is implemented
Hym::Texture loadTextureAssimp(aiMaterial* ai_mat, aiTextureType type, const std::string& tex_name, const glm::vec4& scalar, Hym::TextureType ttype, Hym::ResourceManager& manager)
{
	if (ai_mat->GetTextureCount(type) == 0)
	{
		HYM_ERROR("Mesh had no {} texture", tex_name);
	}
	aiString path;
	if (AI_SUCCESS == ai_mat->GetTexture(type, 0, &path))
	{
		std::string p(path.C_Str());
		return manager.LoadTexture(p,ttype);
	}
	else
	{
		HYM_ERROR("Something went wrong loading the texture {}", path.C_Str());
	}
}

Hym::Material Hym::ResourceManager::createMaterial(aiMaterial* ai_mat)
{
	//auto diffuse = loadTextureAssimp(ai_mat, aiTextureType_DIFFUSE, "albedo", glm::vec4(1, 1, 1, 1),TextureType::Albedo,*this);
	//auto emissive = load_texture_assimp(ai_mat, aiTextureType_EMISSIVE, "emissive", glm::vec4(0, 0, 0, 1));
	//auto specular = load_texture_assimp(ai_mat, aiTextureType_UNKNOWN, "specular", glm::vec4(255, 255, 0, 1));
	//auto normal = load_texture_assimp(ai_mat, aiTextureType_NORMALS, "normal", glm::vec4(128, 128, 255, 255));
	//return { .albedo = albedoTexs.textures.size() -1 };// , normal, specular, emissive
	HYM_WARN("Material loading not implemented");
	return { 0 };
}

void Hym::ResourceManager::upload(std::vector<Vertex>& vertices, std::vector<u32>& indices)
{
	auto vref = ArrayRef<Vertex>::MakeRef(vertices);
	auto iref = ArrayRef<u32>::MakeRef(indices);
	if (!globalVertexBuffer.GetBuffer() && !globalIndexBuffer.GetBuffer())
	{
		globalVertexBuffer = StructuredBuffer<Vertex>("Global Vertex Buffer", vref, dl::BIND_VERTEX_BUFFER | dl::BIND_RAY_TRACING | dl::BIND_SHADER_RESOURCE);
		globalIndexBuffer = StructuredBuffer<u32>("Global Index Buffer", iref,
			dl::BIND_INDEX_BUFFER | dl::BIND_RAY_TRACING | dl::BIND_SHADER_RESOURCE);
	}

	else
	{
		globalVertexBuffer.Add(vref);
		globalIndexBuffer.Add(iref);
	}

}



Hym::Mesh Hym::ResourceManager::createMesh(u64 vAt, u64 iAt, u64 indexSize, u64 verticesSize, const char* name)
{
	Mesh m;
	m.numIndices = indexSize;
	m.offsetIndex = iAt;
	m.numVertices = verticesSize;
	m.offsetVertex = vAt;
	m.name = fmt::format("{}", name);
	
	return m;
}

Diligent::RefCntAutoPtr<Diligent::IBuffer> Hym::ResourceManager::createBLASForAll()
{

}


void Hym::ResourceManager::LoadSceneFile(const std::string& sceneFile, const std::string& sceneAlias)
{
	Assimp::Importer imp;
	const aiScene* scene = imp.ReadFile(sceneFile, aiProcess_Triangulate | aiProcess_GenNormals | aiProcess_CalcTangentSpace | aiProcess_FlipWindingOrder);
	if (scene)
	{
		std::vector<ModelComponent> models;
		std::vector<Vertex> vertices;
		std::vector<u32> indices;
		preTransformNode(*scene->mRootNode, *scene, models,vertices,indices);
		upload(vertices, indices);
		aliasMap[sceneAlias] = std::move(models);
	}

	else
	{
		HYM_WARN("Something went wrong reading the scene file {}", sceneFile);
	}

}

const Hym::ModelComponent* Hym::ResourceManager::GetModel(const std::string& sceneAlias, const std::string& objName) const
{
	auto vec = GetSceneModels(sceneAlias);
	if (!vec)return nullptr;
	auto& vecRef = *vec;
	for (auto& comp : vecRef)
	{
		if (comp.mesh.name == objName)
		{
			return &comp;
		}
	}
	return nullptr;
}

const std::vector<Hym::ModelComponent>* Hym::ResourceManager::GetSceneModels(const std::string& sceneAlias) const
{
	auto it = aliasMap.find(sceneAlias);
	if (it != aliasMap.end())
	{
		auto& vec = it->second;
		return &vec;
	}
	return nullptr;
}

#include "ResourceManager.h"
#include "Debug.h"
#include "glm/mat4x4.hpp"
#include "fmt/format.h"
#include "TextureLoader.h"
#include "Align.hpp"
#include "GlobalState.h"
#include <limits>

void createVertices(aiMesh* p_assMesh, const aiMatrix4x4& trans, std::vector<Hym::Vertex>& vertices, Hym::u32 matIdx)
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
		glm::vec3 normal { assNormal.x, assNormal.y, assNormal.z };
		glm::vec3 tangent{ assTangent.x, assTangent.y, assTangent.z };
		glm::vec3 bitangent{ assBitangent.x,assBitangent.y,assBitangent.z };


		v.normal = glm::normalize(normal);
		//v.tangent = glm::normalize(tangent);
		//v.bitangent = glm::normalize(bitangent);

		auto assUV = p_assMesh->mTextureCoords[0][i];
		glm::vec2 uvMap{ assUV.x,1 - assUV.y };
		v.uv = uvMap;
		//v.matIdx = matIdx;

		vertices.push_back(v);
	}
	// Align for building BLAS
	assert(Hym::Dev->GetAdapterInfo().RayTracing.VertexBufferAlignment == 1); // For now. 
	/*auto alignment = Hym::Dev->GetAdapterInfo().RayTracing.VertexBufferAlignment;
	auto toAdd = Diligent::AlignUp(p_assMesh->mNumVertices * unsigned int(sizeof(Hym::Vertex)), alignment) / sizeof(Hym::Vertex) - p_assMesh->mNumVertices;
	for (int i = 0; i < toAdd; i++)
	{
		vertices.push_back({});
	}*/
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
	assert(Hym::Dev->GetAdapterInfo().RayTracing.VertexBufferAlignment == 1); // For now. 
	/*auto alignment = Hym::Dev->GetAdapterInfo().RayTracing.IndexBufferAlignment;
	auto toAdd = Diligent::AlignUp(count * unsigned int(sizeof(Hym::u32)), alignment) / sizeof(Hym::u32) - count;
	for (int i = 0; i < toAdd; i++)
	{
		indices.push_back(0);
	}*/
}

Hym::Texture Hym::ResourceManager::GetTexture(u64 id, TextureType ttype)
{
	switch (ttype)
	{
	case TextureType::Albedo:
		return { .handle = albedoTexs[id],.offset = id, .textureType = ttype };
	case TextureType::Normal:
		return { .handle = normalTexs[id],.offset = id, .textureType = ttype };
	case TextureType::MetalRough:
		return { .handle = metalRoughnessTexs[id],.offset = id, .textureType = ttype };
	case TextureType::Emissive:
		return { .handle = emissiveTexs[id],.offset = id, .textureType = ttype };
	default:
		HYM_WARN("Texture Type unknown");
		return { .handle = albedoTexs[0],.offset = 0, .textureType = ttype };
	}
}

void Hym::ResourceManager::Init()
{
	//globalVertexBuffer = StructuredBuffer<Vertex>("Global Vertex Buffer", dl::BIND_VERTEX_BUFFER | dl::BIND_RAY_TRACING | dl::BIND_SHADER_RESOURCE);
	//globalIndexBuffer = StructuredBuffer<u32>("Global Index Buffer", dl::BIND_INDEX_BUFFER | dl::BIND_RAY_TRACING | dl::BIND_SHADER_RESOURCE);
}

void Hym::ResourceManager::preTransformNode(const aiNode& node, const aiScene& scene, std::vector<Hym::ModelComponent>& models, std::vector<Vertex>& vertices, std::vector<u32>& indices, 
	std::vector<Material>& materials)
{
	for (unsigned int i = 0; i < node.mNumChildren; i++)
	{
		aiNode& child = *node.mChildren[i];
		child.mTransformation = node.mTransformation * child.mTransformation;
		preTransformNode(child, scene, models,vertices,indices,materials);
		for (unsigned int j = 0; j < child.mNumMeshes; j++)
		{
			auto* mesh = scene.mMeshes[child.mMeshes[j]];
			u64 globalVerticesAt = globalVertexBuffer.GetSize() + vertices.size();
			u64 globalIndicesAt = globalIndexBuffer.GetSize() + indices.size();
			auto mat = createMaterial(scene.mMaterials[mesh->mMaterialIndex], std::string(mesh->mName.C_Str()), materials);
			createVertices(mesh, child.mTransformation,vertices, mat);
			createIndices(mesh,indices);
			//globalVertexBuffer.Add(ArrayRef<Vertex>::MakeRef(vertices));
			//globalIndexBuffer.Add(ArrayRef<u32>::MakeRef(indices));
			auto indexSize = (globalIndexBuffer.GetSize() + indices.size()) - globalIndicesAt;
			auto verticesSize = (globalVertexBuffer.GetSize() + vertices.size()) - globalVerticesAt;
			auto mesh_ = createMesh(globalVerticesAt, globalIndicesAt, indexSize, verticesSize, mesh->mName.C_Str());

			calcMinMax(globalVerticesAt, vertices);

			ModelComponent modelComp;
			modelComp.matIdx = mat;
			modelComp.mesh = mesh_;
			models.push_back(modelComp);
		}
	}
}

void Hym::ResourceManager::calcMinMax(const Hym::u64& globalVerticesAt, std::vector<Hym::Vertex>& vertices)
{
	glm::vec3 min{ std::numeric_limits<float>::max(),std::numeric_limits<float>::max(),std::numeric_limits<float>::max() };
	glm::vec3 max{ std::numeric_limits<float>::min(),std::numeric_limits<float>::min(),std::numeric_limits<float>::min() };

	for (int i = globalVerticesAt; i < vertices.size(); i++)
	{
		auto& vertex = vertices[i];
		if (vertex.pos.x > max.x) max.x = vertex.pos.x;
		if (vertex.pos.x < min.x) min.x = vertex.pos.x;
		if (vertex.pos.y < min.y) min.y = vertex.pos.y;
		if (vertex.pos.y > max.y) max.y = vertex.pos.y;
		if (vertex.pos.z < min.z) min.z = vertex.pos.z;
		if (vertex.pos.z > max.z) max.z = vertex.pos.z;
	}

	minMaxVec.push_back({ min,max });
}

Hym::Texture Hym::ResourceManager::LoadTexture(const std::string& filename, const std::string& alias, Hym::TextureType type)
{
	if (textureAliasMap.contains(alias))
	{
		HYM_WARN("Texture {} already exists, using from cache", alias);
		auto tex = textureAliasMap[alias];
		return tex;
	}

	dl::TextureLoadInfo tlinfo;
	tlinfo.GenerateMips = true;
	tlinfo.IsSRGB = true;
	RefCntAutoPtr<dl::ITextureLoader> loader;
	RefCntAutoPtr<dl::ITexture> tex;
	dl::CreateTextureLoaderFromFile(filename.c_str(), dl::IMAGE_FILE_FORMAT_UNKNOWN, tlinfo, &loader);
	loader->CreateTexture(Dev, &tex);
	Texture out;
	TexArray* specificTA;


	switch (type)
	{
	case TextureType::Albedo:
	{		
		out = Texture{ .handle = tex, .offset = albedoTexs.size(), .textureType = type };
		specificTA = &albedoTexs;
		break;
	}
	case TextureType::Normal:
	{
		out = Texture{ .handle = tex, .offset = normalTexs.size(), .textureType = type };
		specificTA = &normalTexs;
		break;
	}
	case TextureType::MetalRough:
	{
		out = Texture{ .handle = tex, .offset = metalRoughnessTexs.size(), .textureType = type };		
		specificTA = &metalRoughnessTexs;
		break;
	}
	case TextureType::Emissive:
	{
		out = Texture{ .handle = tex, .offset = emissiveTexs.size(), .textureType = type };
		specificTA = &emissiveTexs;
		break;
	}
	}

	specificTA->push_back(out.handle);
	textureAliasMap[alias] = out;

	return out;
}

// TODO: Transform the errors into warnings when default texture is implemented
Hym::Texture loadTextureAssimp(aiMaterial* ai_mat, const std::string& meshName, aiTextureType type, const std::string& tex_name, const std::string& scenePath, Hym::TextureType ttype, Hym::ResourceManager& manager)
{
	if (ai_mat->GetTextureCount(type) == 0)
	{
		HYM_WARN("Mesh {} has no {} texture", meshName, tex_name);
		return Hym::Texture{ .handle = {},.offset = 0,.textureType = Hym::TextureType::Albedo };
	}
	aiString path;
	if (AI_SUCCESS == ai_mat->GetTexture(type, 0, &path))
	{
		std::filesystem::path p(path.C_Str());
		if (p.is_absolute())
		{
			return manager.LoadTexture(p.string(), p.string(), ttype);
		}
		else
		{
			std::string p = scenePath + "/" + path.C_Str();
			return manager.LoadTexture(p, p, ttype);
		}
	}
	else
	{
		HYM_ERROR("Something went wrong loading the texture {}", path.C_Str());
	}
}

Hym::u32 Hym::ResourceManager::createMaterial(aiMaterial* ai_mat, const std::string& meshName, std::vector<Material>& materials)
{
	//auto diffuse = loadTextureAssimp(ai_mat, aiTextureType_DIFFUSE, "albedo", glm::vec4(1, 1, 1, 1),TextureType::Albedo,*this);
	//auto emissive = load_texture_assimp(ai_mat, aiTextureType_EMISSIVE, "emissive", glm::vec4(0, 0, 0, 1));
	//auto specular = load_texture_assimp(ai_mat, aiTextureType_UNKNOWN, "specular", glm::vec4(255, 255, 0, 1));
	//auto normal = load_texture_assimp(ai_mat, aiTextureType_NORMALS, "normal", glm::vec4(128, 128, 255, 255));
	//return { .albedo = albedoTexs.textures.size() -1 };// , normal, specular, emissive
	auto addMat = [&](u32& offset, aiTextureType type, const char* name, TextureType ttype) {
		auto tex = loadTextureAssimp(ai_mat, meshName, type, name,currSceneFileDir.string(), ttype, *this);
		if (tex.handle)
		{
			offset = tex.offset;
			return true;
		}
		else
		{
			offset = GetMaxIdx();
			return false;
		}
	};
	Material mat;
	bool hasAtLeastOneTex = addMat(mat.albedo, aiTextureType_DIFFUSE, "albedo", TextureType::Albedo);
	if (hasAtLeastOneTex)
	{
		materials.push_back(mat);
		return materials.size() - 1;
	}
	else return GetMaxIdx();
}

//#include "Debug.h"

bool Hym::ResourceManager::upload(const std::vector<Vertex>& vertices, const std::vector<u32>& indices, const std::vector<Material>& materials)
{
	auto vref = ArrayRef<Vertex>::MakeRef(vertices);
	auto iref = ArrayRef<u32>::MakeRef(indices);
	auto matref = ArrayRef<Material>::MakeRef(materials);
	bool rebuild = false;
	if (!globalVertexBuffer.GetBuffer() && !globalIndexBuffer.GetBuffer())
	{
		globalVertexBuffer = StructuredBuffer<Vertex>("Global Vertex Buffer", vref, dl::BIND_VERTEX_BUFFER | dl::BIND_RAY_TRACING | dl::BIND_SHADER_RESOURCE);
		globalIndexBuffer = StructuredBuffer<u32>("Global Index Buffer", iref,
			dl::BIND_INDEX_BUFFER | dl::BIND_RAY_TRACING | dl::BIND_SHADER_RESOURCE);
		if(!materials.empty())
			globalMaterialBuffer = StructuredBuffer<Material>("Global Material Indices", matref, dl::BIND_SHADER_RESOURCE);
	}

	else
	{
		rebuild = globalVertexBuffer.Add(vref);
		rebuild = globalIndexBuffer.Add(iref) || rebuild;
		if(!materials.empty())
			rebuild = globalMaterialBuffer.Add(matref) || rebuild;
	}

	return rebuild;

}



Hym::Mesh Hym::ResourceManager::createMesh(u64 vAt, u64 iAt, u64 indexSize, u64 verticesSize, const char* name)
{
	Mesh m;
	m.numIndices = indexSize;
	m.offsetIndex = iAt;
	m.numVertices = verticesSize;
	m.offsetVertex = vAt;
	m.idxIntoLinearBuffer = meshIdx++;
	meshNames.push_back(fmt::format("{}", name));
	//m.name = fmt::format("{}", name);	
	return m;
}

void Hym::ResourceManager::createBLAS(std::vector<ModelComponent>& models)
{
	for (auto& comp : models)
	{
		RefCntAutoPtr<dl::IBottomLevelAS> blas;
		auto& m = comp.mesh;
		auto& name = meshNames[m.idxIntoLinearBuffer];

		dl::BLASTriangleDesc tdesc;
		tdesc.MaxVertexCount = m.numVertices;
		tdesc.VertexValueType = dl::VT_FLOAT32;
		tdesc.VertexComponentCount = 3;
		tdesc.MaxPrimitiveCount = m.numIndices / 3;
		tdesc.IndexType = dl::VT_UINT32;
		tdesc.GeometryName = name.c_str();

		dl::BottomLevelASDesc adesc;
		adesc.Flags = dl::RAYTRACING_BUILD_AS_PREFER_FAST_TRACE;
		adesc.pTriangles = &tdesc;
		adesc.TriangleCount = 1;
		adesc.Name = name.c_str();

		Dev->CreateBLAS(adesc, &blas);

		// This shows how to create a blas
		// https://github.com/DiligentGraphics/DiligentSamples/blob/master/Tutorials/Tutorial22_HybridRendering/src/Tutorial22_HybridRendering.cpp
		if (!blasScratchBuffer || blasScratchBuffer->GetDesc().Size < blas->GetScratchBufferSizes().Build)
		{
			dl::BufferDesc BuffDesc;
			BuffDesc.Name = "BLAS Scratch Buffer";
			BuffDesc.Usage = dl::USAGE_DEFAULT;
			BuffDesc.BindFlags = dl::BIND_RAY_TRACING;
			BuffDesc.Size = blas->GetScratchBufferSizes().Build;

			blasScratchBuffer = nullptr;
			Dev->CreateBuffer(BuffDesc, nullptr, &blasScratchBuffer);
		}

		dl::BLASBuildTriangleData TriangleData;
		TriangleData.GeometryName = tdesc.GeometryName;
		TriangleData.pVertexBuffer = globalVertexBuffer.GetBuffer();
		TriangleData.VertexStride = globalVertexBuffer.GetBuffer()->GetDesc().ElementByteStride;
		TriangleData.VertexOffset = m.offsetVertex * TriangleData.VertexStride;
		TriangleData.VertexCount = m.numVertices;
		TriangleData.VertexValueType = tdesc.VertexValueType;
		TriangleData.VertexComponentCount = tdesc.VertexComponentCount;
		TriangleData.pIndexBuffer = globalIndexBuffer.GetBuffer();
		TriangleData.IndexOffset = m.offsetIndex * globalIndexBuffer.GetBuffer()->GetDesc().ElementByteStride;
		TriangleData.PrimitiveCount = tdesc.MaxPrimitiveCount;
		TriangleData.IndexType = tdesc.IndexType;
		TriangleData.Flags = dl::RAYTRACING_GEOMETRY_FLAG_OPAQUE;

		dl::BuildBLASAttribs Attribs;
		Attribs.pBLAS = blas;
		Attribs.pTriangleData = &TriangleData;
		Attribs.TriangleDataCount = 1;

		Attribs.pScratchBuffer = blasScratchBuffer;
		Attribs.BLASTransitionMode = dl::RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
		Attribs.GeometryTransitionMode = dl::RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
		Attribs.ScratchBufferTransitionMode = dl::RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
		Imm->BuildBLAS(Attribs);
		blasVec.push_back(std::move(blas));
	}
}

std::vector<Hym::dl::IDeviceObject*> Hym::ResourceManager::createViews(std::vector<RefCntAutoPtr<dl::ITexture>>& texs)
{
	std::vector<dl::IDeviceObject*> views;
	views.reserve(texs.size());
	for (auto& tex : texs)
	{
		views.push_back(tex->GetDefaultView(dl::TEXTURE_VIEW_SHADER_RESOURCE));
	}
	return views;
}

//void Hym::ResourceManager::createIdxIntoLinearBuffer(std::vector<ModelComponent>& models)
//{
//	for (auto& m : models)
//	{
//		m.mesh.idxIntoLinearBuffer = meshIdx++;
//	}
//}


void Hym::ResourceManager::LoadSceneFile(const std::string& sceneFile, const std::string& sceneAlias)
{
	if (aliasMap.contains(sceneAlias))
		return; // Already loaded.
	Assimp::Importer imp;

	currSceneFileDir = std::filesystem::path(sceneFile).parent_path();
	const aiScene* scene = imp.ReadFile(sceneFile, aiProcess_Triangulate | aiProcess_GenNormals | aiProcess_CalcTangentSpace | aiProcess_FlipWindingOrder);
	if (scene)
	{
		std::vector<ModelComponent> models;
		std::vector<Vertex> vertices;
		std::vector<u32> indices;
		std::vector<Material> materials;
		std::vector<ObjectAttrs> attrs;
		preTransformNode(*scene->mRootNode, *scene, models,vertices,indices,materials);
		upload(vertices, indices, materials);
		createBLAS(models);
		aliasMap[sceneAlias] = std::move(models);
		//createIdxIntoLinearBuffer(models);
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
		auto& name = meshNames[comp.mesh.idxIntoLinearBuffer];
		if (name == objName)
		{
			return &comp;
		}
	}
	return nullptr;
}

const Hym::Texture* Hym::ResourceManager::GetTexture(const std::string& name)
{
	auto it = textureAliasMap.find(name);
	if (it != textureAliasMap.end())
		return &it->second;
	else return nullptr;
}

//bool Hym::ResourceManager::CreateNewModel(const std::string& objName, const ModelComponent& mc) const
//{
//	return false;
//}

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

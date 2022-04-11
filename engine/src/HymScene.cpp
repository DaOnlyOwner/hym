#include "HymScene.h"
#include "fmt/format.h"
// Various code snippets for building the TLAS are from 
// https://github.com/DiligentGraphics/DiligentSamples/blob/master/Tutorials/Tutorial22_HybridRendering/src/Tutorial22_HybridRendering.cpp

//void Hym::Scene::FinishedStaticObjects(u32 estimatedAmountDynamicObjs)
//{
//	// Enough space for 10k dynamic objects
//	std::vector<ObjectAttrs> attrs;
//	auto view = reg.view<ModelComponent, TransformComponent>();
//	u32 countObjs = 0;
//	for (auto e : view)
//	{
//		auto& model = view.get<ModelComponent>(e);
//		auto& trans = view.get<TransformComponent>(e);
//
//		auto [modelM, normalM] = trans.GetModel_Normal();
//		ObjectAttrs attr;
//		attr.firstIndex = model.mesh.offsetIndex;
//		attr.firstVertex = model.mesh.offsetVertex;
//		attr.matIdx = model.matIdx;
//		attrs.push_back(attr);
//		countObjs++;
//	}
//	instanceAttrs = StructuredBuffer<ObjectAttrs>("Object Attrs", countObjs+estimatedAmountDynamicObjs, dl::BIND_SHADER_RESOURCE);
//	auto ref = ArrayRef<ObjectAttrs>::MakeRef(attrs);
//	instanceAttrs.Add(ref, 1);	
//	addingStacicScene = false;
//}

Hym::Concept Hym::Scene::AddConcept(const Concept& c, const std::string& name)
{
	activeConcepts++;
	auto out = c.Spawn(reg);
	DataComponent data;
	addName(data, name, out.GetID());

	auto tc = out.GetComponent<TransformComponent>();
	auto mc = out.GetComponent<ModelComponent>();
	if (tc && mc)
	{
		addInstanceIdx(data);
		ObjectAttrs attr[1];
		auto [modelM, normalM] = tc->GetModel_Normal();
		attr[0].normalMat = glm::transpose(normalM);
		attr[0].firstIndex = mc->mesh.offsetIndex;
		attr[0].firstVertex = mc->mesh.offsetVertex;
		attr[0].matIdx = mc->matIdx;
		auto ref = ArrayRef<ObjectAttrs>::MakeRef(attr, _countof(attr));
		instanceAttrs.Add(ref, 1.5f); // TODO: If remove <- here update instead of Add...
	}
	out.AddComponent<DataComponent>(data);
	
	if(tc && mc) 
		rebuildTLAS();
	return out;
}

std::vector<Hym::Concept> Hym::Scene::AddConcepts(const ArrayRef<std::pair<Concept, std::string>>& concepts)
{
	std::vector<ObjectAttrs> attrs;
	attrs.reserve(concepts.size);
	std::vector<Concept> out;
	out.reserve(concepts.size);
	for (int i = 0; i < concepts.size; i++)
	{
		auto& [c,name] = concepts.data[i];
		auto new_c = c.Spawn(reg);
		DataComponent data;
		
		addName(data, name, new_c.GetID());


		auto tc = new_c.GetComponent<TransformComponent>();
		auto mc = new_c.GetComponent<ModelComponent>();
		if (tc && mc)
		{
			auto [modelM, normalM] = tc->GetModel_Normal();

			addInstanceIdx(data);
			ObjectAttrs attr;
			attr.normalMat = glm::transpose(normalM);
			attr.firstIndex = mc->mesh.offsetIndex;
			attr.firstVertex = mc->mesh.offsetVertex;
			attr.matIdx = mc->matIdx;
			attrs.push_back(attr);
		}
		new_c.AddComponent<DataComponent>(data);
		out.push_back(new_c);
		activeConcepts++;
	}
	if (!attrs.empty())
	{
		auto ref = ArrayRef<ObjectAttrs>::MakeRef(attrs);
		instanceAttrs.Add(ref, 1.5f);
		auto i = sizeof(ObjectAttrs);
		rebuildTLAS();
	}
	return out;
}

void Hym::Scene::DelConcept(const Concept& c)
{
	reg.destroy(c.GetID());

	reusableIndices.push_back(c.GetComponent<DataComponent>()->objAttrIdx);

	activeConcepts--;
	rebuildTLAS();
}

void Hym::Scene::DelConcepts(ArrayRef<Concept>& c)
{
	for (int i = 0; i < c.size; i++)
	{
		auto& con = c.data[i];
		reg.destroy(con.GetID());
		reusableIndices.push_back(con.GetComponent<DataComponent>()->objAttrIdx);
		activeConcepts--;
	}
	rebuildTLAS();
}

void Hym::Scene::UpdateDynamicObjs()
{
	updateTLAS();
	dynamicObjs.clear();
}

std::pair<glm::vec3,glm::vec3> Hym::Scene::GetMinMax() const
{
	auto v = reg.view<TransformComponent,ModelComponent>();
	glm::vec3 min = {std::numeric_limits<float>::max(),std::numeric_limits<float>::max() ,std::numeric_limits<float>::max() };
	glm::vec3 max = {std::numeric_limits<float>::min(),std::numeric_limits<float>::min() ,std::numeric_limits<float>::min() };
	for (auto e : v)
	{
		auto& tc = reg.get<TransformComponent>(e);
		auto& mc = reg.get<ModelComponent>(e);
		auto [modelM, normalM] = tc.GetModel_Normal();
		auto [min_, max_] = resourceManager.GetMinMax(mc.mesh.idxIntoLinearBuffer);
		auto minTrans = modelM * glm::vec4(min_,1);
		auto maxTrans = modelM * glm::vec4(max_,1);

		if (maxTrans.x > max.x) max.x = maxTrans.x;
		if (minTrans.x < min.x) min.x = minTrans.x;
		if (minTrans.y < min.y) min.y = minTrans.y;
		if (maxTrans.y > max.y) max.y = maxTrans.y;
		if (minTrans.z < min.z) min.z = minTrans.z;
		if (maxTrans.z > max.z) max.z = maxTrans.z;
	}

	return { min,max };

}

void Hym::Scene::rebuildTLAS()
{

	instanceData.clear();
	names.clear();
	instanceData.reserve(activeConcepts);
	names.reserve(activeConcepts);

	auto group = reg.group<TransformComponent,ModelComponent,DataComponent>();
	int countObjs = 0;
	for (auto e : group)
	{
		auto [trans, model, data] = group.get<TransformComponent, ModelComponent,DataComponent>(e);
		fillTLASInstanceData(trans,model,data, instanceData);
		countObjs++;
	}

	createTLAS(countObjs);
	createTLASScratchBuffer();

	createTLASInstanceBuffer(countObjs);

	buildTLAS(instanceData);
}

void Hym::Scene::buildTLAS(std::vector<Diligent::TLASBuildInstanceData>& instanceData)
{
	dl::BuildTLASAttribs Attribs;
	Attribs.pTLAS = tlas;
	Attribs.Update = false;

	Attribs.pScratchBuffer = tlasScratchBuffer;

	Attribs.pInstanceBuffer = tlasInstanceBuffer;

	Attribs.pInstances = instanceData.data();
	Attribs.InstanceCount = instanceData.size();

	Attribs.TLASTransitionMode = dl::RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
	Attribs.BLASTransitionMode = dl::RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
	Attribs.InstanceBufferTransitionMode = dl::RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
	Attribs.ScratchBufferTransitionMode = dl::RESOURCE_STATE_TRANSITION_MODE_TRANSITION;

	Imm->BuildTLAS(Attribs);
}

void Hym::Scene::createTLASInstanceBuffer(int countObjs)
{
	tlasInstanceBuffer.Release();
	dl::BufferDesc BuffDesc;
	BuffDesc.Name = "TLAS Instance Buffer";
	BuffDesc.Usage = dl::USAGE_DEFAULT;
	BuffDesc.BindFlags = dl::BIND_RAY_TRACING;
	BuffDesc.Size = dl::TLAS_INSTANCE_DATA_SIZE * countObjs;
	Dev->CreateBuffer(BuffDesc, nullptr, &tlasInstanceBuffer);
}

void Hym::Scene::createTLAS(int countObjs)
{
	tlas.Release();
	dl::TopLevelASDesc tlasDesc;
	tlasDesc.Name = "TLAS";
	tlasDesc.MaxInstanceCount = countObjs;
	tlasDesc.Flags = dl::RAYTRACING_BUILD_AS_ALLOW_UPDATE | dl::RAYTRACING_BUILD_AS_PREFER_FAST_TRACE;
	Dev->CreateTLAS(tlasDesc, &tlas);
}

glm::mat4x3 Hym::Scene::fillTLASInstanceData(const TransformComponent& trans, const ModelComponent& model, const DataComponent& data, std::vector<Diligent::TLASBuildInstanceData>& instanceData)
{
	dl::TLASBuildInstanceData idata;

	auto [modelM, normal] = trans.GetModel_Normal();

	idata.CustomId = data.objAttrIdx;
	idata.Mask = 0xFF;
	dl::InstanceMatrix mat;
	for (int x = 0; x < 3; x++)
	{
		for (int y = 0; y < 4; y++)
		{
			mat.data[x][y] = modelM[y][x];
		}
	}

	auto& mesh = model.mesh;

	idata.Transform = mat;
	idata.pBLAS = this->resourceManager.GetBlas(mesh.idxIntoLinearBuffer);

	names.push_back(data.name);

	idata.InstanceName = names.back().c_str();
	instanceData.push_back(idata);
	return normal;
}

void Hym::Scene::createTLASScratchBuffer()
{
	tlasScratchBuffer.Release();
	dl::BufferDesc BuffDesc;
	BuffDesc.Name = "TLAS Scratch Buffer";
	BuffDesc.Usage = dl::USAGE_DEFAULT;
	BuffDesc.BindFlags = dl::BIND_RAY_TRACING;
	BuffDesc.Size = std::max(tlas->GetScratchBufferSizes().Build, tlas->GetScratchBufferSizes().Update);
	Dev->CreateBuffer(BuffDesc, nullptr, &tlasScratchBuffer);
}

void Hym::Scene::addInstanceIdx(DataComponent& data)
{
	if (reusableIndices.empty())
		data.objAttrIdx = currInstanceIdx++;
	else
	{
		data.objAttrIdx = reusableIndices.back();
		reusableIndices.pop_back();
	}
}

void Hym::Scene::addName(DataComponent& data, const std::string& name, entt::entity e)
{
	if (name == "")
		data.name = fmt::format("{:x}", reg.size());
	else data.name = name;
	nameEntityMap[data.name] = e;
}

void Hym::Scene::updateTLAS()
{

	instanceData.clear();
	names.clear();
	instanceData.reserve(activeConcepts);
	names.reserve(activeConcepts);

	int countDynamicObjs = 0;
	for (auto entity : dynamicObjs)
	{

		auto& trans = reg.get<TransformComponent>(entity);
		auto& model = reg.get<ModelComponent>(entity);
		auto& data = reg.get<DataComponent>(entity);
		auto normalM = fillTLASInstanceData(trans, model, data, instanceData);

		ObjectAttrs attr
		{
			.normalMat = glm::mat4(normalM),
			.firstIndex = (u32)model.mesh.offsetIndex,
			.firstVertex = (u32)model.mesh.offsetVertex,
			.matIdx = model.matIdx
		};

		instanceAttrs.Update(data.objAttrIdx, attr);
		countDynamicObjs++;
	}

	dl::BuildTLASAttribs Attribs;
	Attribs.pTLAS = tlas;
	Attribs.Update = true;

	// Scratch buffer will be used to store temporary data during TLAS build or update.
	// Previous content in the scratch buffer will be discarded.
	Attribs.pScratchBuffer = tlasScratchBuffer;

	// Instance buffer will store instance data during TLAS build or update.
	// Previous content in the instance buffer will be discarded.
	Attribs.pInstanceBuffer = tlasInstanceBuffer;

	// Instances will be converted to the format that is required by the graphics driver and copied to the instance buffer.
	Attribs.pInstances = instanceData.data();
	Attribs.InstanceCount = instanceData.size();

	// Allow engine to change resource states.
	Attribs.TLASTransitionMode = dl::RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
	Attribs.BLASTransitionMode = dl::RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
	Attribs.InstanceBufferTransitionMode = dl::RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
	Attribs.ScratchBufferTransitionMode = dl::RESOURCE_STATE_TRANSITION_MODE_TRANSITION;

	Imm->BuildTLAS(Attribs);
}

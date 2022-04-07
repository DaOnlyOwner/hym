#pragma once
#include <string>
#include "Components.h"
#include <vector>
#include "ResourceManager.h"
#include "Concept.h"
#include "TopLevelAS.h"
#include "Definitions.h"
#include "Sun.h"
#include <unordered_map>

namespace Hym
{
	class Scene
	{
	public:
		Scene(ResourceManager& manager)
			:resourceManager(manager), dynamicObjs(reg, entt::collector.update<TransformComponent>()), sun({90,-96}, { 1,1,1 })
		{
			instanceAttrs = StructuredBuffer<ObjectAttrs>("Instance Attr Buffer", dl::BIND_SHADER_RESOURCE);
		}

		Concept AddConcept(const Concept& c, const std::string& name = "");
		std::vector<Hym::Concept> AddConcepts(const ArrayRef<std::pair<Concept, std::string>>& concepts);
		void DelConcept(const Concept& c);
		void DelConcepts(ArrayRef<Concept>& c);
		void Serialize(const std::string& filename);
		void Deserialize(const std::string& filename);
		void UpdateDynamicObjs();
		StructuredBuffer<ObjectAttrs>& GetInstanceObjAttrsBuffer() { return instanceAttrs; }
		std::pair<glm::vec3,glm::vec3> GetMinMax() const;
		ResourceManager& GetResourceManager() { return resourceManager; }
		entt::registry& GetRegistry() { return reg; }
		Sun& GetSun() { return sun; }
		const entt::registry& GetRegistry() const { return reg; }
		dl::ITopLevelAS* GetTLAS() { return tlas.RawPtr(); }

	private:
		void updateTLAS();
		void rebuildTLAS();

		void buildTLAS(std::vector<Diligent::TLASBuildInstanceData>& instanceData);

		void createTLASInstanceBuffer(int countObjs);

		void createTLAS(int countObjs);

		glm::mat4x3 fillTLASInstanceData(const TransformComponent& trans, const ModelComponent& model, const DataComponent& data, std::vector<Diligent::TLASBuildInstanceData>& instanceData);

		void createTLASScratchBuffer();
		void addInstanceIdx(DataComponent& data);
		void addName(DataComponent& data, const std::string& name, entt::entity e);


	private:
		ResourceManager& resourceManager;
		entt::registry reg;
		entt::observer dynamicObjs;
		Sun sun;
		bool addingStacicScene = true;
		
		RefCntAutoPtr<dl::ITopLevelAS> tlas;
		RefCntAutoPtr<dl::IBuffer> tlasScratchBuffer;
		RefCntAutoPtr<dl::IBuffer> tlasInstanceBuffer;
		std::vector<dl::TLASBuildInstanceData> instanceData;
		std::vector<std::string> names;

		std::unordered_map<std::string, entt::entity> nameEntityMap;

		StructuredBuffer<ObjectAttrs> instanceAttrs;

		std::vector<u32> reusableIndices;
		u32 currInstanceIdx = 0;

		u64 activeConcepts = 0;

	};
}
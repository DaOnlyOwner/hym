#pragma once
#include <string>
#include "Components.h"
#include <vector>
#include "ResourceManager.h"
#include "Concept.h"

namespace Hym
{
	class Scene
	{
	public:
		Scene(ResourceManager& manager)
			:resourceManager(manager)
		{

		}

		Concept AddConcept(const Concept& c);
		void DelConcept(const Concept& c);
		void Serialize(const std::string& filename);
		void Deserialize(const std::string& filename);
		ResourceManager& GetResourceManager() { return resourceManager; }
		entt::registry& GetRegistry() { return reg; }
		const entt::registry& GetRegistry() const { return reg; }

	private:
		ResourceManager& resourceManager;
		entt::registry reg;
	};
}
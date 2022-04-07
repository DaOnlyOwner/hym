#include "Concept.h"
#include "Components.h"
#define ADD_COMP(comp) addComp<comp>(*reg, other_reg, me, c.me)

template<typename T>
void addComp(const entt::registry& from, entt::registry& to, entt::entity frome, entt::entity toe)
{
	if (from.any_of<T>(frome))
	{
		auto& arg = from.get<T>(frome);
		to.emplace<T>(toe, arg);
	}
}

void Hym::Concept::SetScale(const glm::vec3& s)
{
	reg->patch<TransformComponent>(me, [&](TransformComponent& transform) {transform.scale = s; });
}

void Hym::Concept::SetRotation(const glm::vec3& r)
{
	reg->patch<TransformComponent>(me, [&](TransformComponent& transform) {transform.rotation = r; });
}

void Hym::Concept::SetTranslation(const glm::vec3& t)
{
	reg->patch<TransformComponent>(me, [&](TransformComponent& transform) {transform.scale = t; });
}


Hym::Concept Hym::Concept::Spawn(entt::registry& other_reg) const
{
	Concept c(other_reg);
	ADD_COMP(ModelComponent);
	ADD_COMP(TransformComponent);
	return c;
}


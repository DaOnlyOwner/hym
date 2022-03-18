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

Hym::Concept Hym::Concept::Spawn(entt::registry& other_reg) const
{
	Concept c(other_reg);
	ADD_COMP(ModelComponent);
	ADD_COMP(TransformComponent);
	return c;
}


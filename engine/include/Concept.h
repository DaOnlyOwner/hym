#pragma once
#include "entt/entt.hpp"
#include "Definitions.h"

namespace Hym
{
	class Concept
	{
	public:
		Concept(entt::registry& reg_, entt::entity me)
			:reg(&reg_),me(me){}
		Concept(entt::registry& reg_)
			:reg(&reg_),me(reg_.create())
		{
		}
		Concept()
			:Concept(staticReg())
		{
		}

		template<typename TComp>
		void AddComponent(const TComp& comp)
		{
			reg->emplace_or_replace<TComp>(me, comp);
		}

		template<typename TComp>
		TComp* GetComponent()
		{
			return reg->try_get<TComp>(me);
		}

		template<typename TComp>
		void DelComponent()
		{
			reg->remove<TComp>(me);
		}

		Concept Spawn(entt::registry& other_reg) const ;

		entt::entity GetID() const { return me; }

	private:
		entt::entity me;
		entt::registry& staticReg()
		{
			static entt::registry reg;
			return reg;
		}

		entt::registry* reg = nullptr;


	};
}

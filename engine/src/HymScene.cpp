#include "HymScene.h"

Hym::Concept Hym::Scene::AddConcept(const Concept& c)
{
	return c.Spawn(reg);
}

void Hym::Scene::DelConcept(const Concept& c)
{
	reg.destroy(c.GetID());
}

#pragma once
#include "Definitions.h"
#include "glm/vec3.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/euler_angles.hpp"

namespace Hym
{
	struct Mesh
	{
		u32 numIndices;
		u32 numVertices;
		u32 offsetIndex;
		u32 offsetVertex;
		std::string name;
	};

	struct Material
	{
		u64 albedo;
		u64 normal;
		u64 metalRough;
		u64 emissive;
	};

	struct ModelComponent{
		Material mat;
		Mesh mesh;	
	};

	struct TransformComponent
	{
		glm::vec3 scale;
		glm::vec3 rotation;
		glm::vec3 translation;

		std::pair<glm::mat4, glm::mat4> GetModel_Normal() const
		{
			glm::mat4 m{ 1.0 };
			auto translationM = glm::translate(m, translation);
			auto rotM = glm::yawPitchRoll(rotation.x, rotation.y, rotation.z);
			auto scaleM = glm::scale(m, scale);
			auto model = (translationM * rotM * scaleM);
			auto normal = glm::inverse(glm::transpose(model));
			return { model,normal };
		}

		TransformComponent() :scale(1, 1, 1), rotation(0, 0, 0), translation(0, 0, 0) {}
		TransformComponent(const glm::vec3& scale,
			const glm::vec3& rotation,
			const glm::vec3& translation)
			:scale(scale), rotation(rotation), translation(translation) {}
	};

}

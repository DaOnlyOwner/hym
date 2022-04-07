#pragma once
#include "Definitions.h"
#include "glm/vec3.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/euler_angles.hpp"
#include "Buffer.h"
#include <memory>

namespace Hym
{
	struct Mesh
	{
		u32 numIndices=0;
		u32 numVertices=0;
		u64 offsetIndex=0;
		u64 offsetVertex=0;
		u64 idxIntoLinearBuffer = 0;
		//std::string name="Invalid";
	};

	struct Material
	{
		u32 albedo;
		u32 normal;
		u32 metalRough;
		u32 emissive;
	};

	struct ModelComponent{
		u32 matIdx;
		Mesh mesh;	
	};

	struct DataComponent
	{
		std::string name;
		u32 objAttrIdx;
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

#pragma once

#include <GPUBuffer.h>
#include "Definitions.h"

namespace Hym
{
	class Sun
	{
	public:
		Sun(const glm::vec2& direction, const glm::vec3& color) 
			:Direction(direction),Color(color),sunBuffer("Sun Buffer") {}
		glm::vec2 Direction;
		glm::vec3 Color;
		void Set();
		void RenderShadowMap();
		dl::IBuffer* GetSunBuffer() { return sunBuffer.GetBuffer(); }
		
	private:
		struct SunDataGPU
		{
			glm::vec3 direction;
			float p1;
			glm::vec3 color;
			float p2;
		};
		UniformBuffer<SunDataGPU> sunBuffer;
	};
}
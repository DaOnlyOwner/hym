#pragma once
#include <cstdint>
#include "glm/vec3.hpp"
#include "glm/vec2.hpp"
#include "glm/mat4x4.hpp"
#include "RefCntAutoPtr.hpp"
#include <vector>

namespace Hym
{
	namespace dl = Diligent;
	using dl::RefCntAutoPtr;
	typedef uint32_t u32;
	typedef uint64_t u64;
	typedef int32_t i32;
	typedef int64_t i64;

	struct Vertex
	{
		glm::vec3 pos;
		glm::vec3 normal;
		//glm::vec3 tangent;
		//glm::vec3 bitangent;
		glm::vec2 uv;
	};

	struct ObjectAttrs
	{
		glm::mat4 normalMat;
		u32 firstIndex;
		u32 firstVertex;
		u32 matIdx;
		u32 padding;
	};

	template<typename T>
	struct ArrayRef
	{
		u64 size;
		const T* data;
		static ArrayRef<T> MakeRef(const std::vector<T>& vec)
		{
			return { .size = vec.size(), .data = vec.data() };
		}
		static ArrayRef<T> MakeRef(const std::vector<T>& vec, u64 offset)
		{
			return { .size = vec.size() - offset, .data = vec.data() + offset };
		}
		static ArrayRef<T> MakeRef(const T* data, u64 size)
		{
			return { .size = size, .data = data };
		}
	};

}


#pragma once
#include "Definitions.h"
#include "Buffer.h"
#include "GlobalState.h"
#include <string>
#include "MapHelper.hpp"

namespace Hym
{
	/// <summary>
	/// Implements an structured buffer. The buffer dynamically grows.
	/// Right now, there is no way to delete data
	/// </summary>
	/// <typeparam name="T">The type of the buffer</typeparam>
	template<typename T>
	class StructuredBuffer
	{
	private:
		StructuredBuffer(const std::string& name, dl::BIND_FLAGS bind, u64 size, u64 capacity)
			:name(name),bind(bind),size(size),capacity(size) {}
	public:
		StructuredBuffer() = default;
		StructuredBuffer(const std::string& name, const ArrayRef<T>& a, dl::BIND_FLAGS bind = dl::BIND_FLAGS::BIND_SHADER_RESOURCE)
			:StructuredBuffer(name,bind,a.size,a.size)
		{
			auto desc = getDesc();
			dl::BufferData data;
			data.DataSize = a.size * sizeof(T);
			data.pData = a.data;
			Dev->CreateBuffer(desc, &data, &bufferHandle);
		}

		StructuredBuffer(const std::string& name, u64 capacity, dl::BIND_FLAGS bind = dl::BIND_FLAGS::BIND_SHADER_RESOURCE)
			:StructuredBuffer(name, bind, 0,capacity)
		{
			auto desc = getDesc();
			Dev->CreateBuffer(desc, nullptr, &bufferHandle);
		}
		StructuredBuffer(const std::string& name, dl::BIND_FLAGS bind = dl::BIND_FLAGS::BIND_SHADER_RESOURCE)
			:name(name),bind(bind),capacity(0),size(0)
		{}
		
		bool Add(const ArrayRef<T>& a, float growth = 1.f)
		{
			bool newBufferHandle = false;
			if (size + a.size >= capacity)
			{
				grow(std::ceil((a.size + size) * growth));
				newBufferHandle = true;
			}

			Imm->UpdateBuffer(bufferHandle, size * sizeof(T), a.size * sizeof(T), a.data, dl::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
			size = a.size + size;
			return newBufferHandle;
		}

		dl::IBuffer* GetBuffer()
		{
			return bufferHandle.RawPtr();
		}

		void Reserve(u64 amount)
		{
			if (amount > size)
			{
				grow(amount);
			}
		}

		u64 GetSize()
		{
			return size;
		}

		u64 GetCapacity()
		{
			return capacity;
		}

	private:

		dl::BufferDesc getDesc()
		{
			dl::BufferDesc desc;
			desc.Mode = dl::BUFFER_MODE_STRUCTURED;
			desc.Size = capacity * sizeof(T);
			desc.Name = name.c_str();
			desc.ElementByteStride = sizeof(T);
			desc.BindFlags = bind;
			return desc;
		}

		void grow(u64 newCapacity)
		{
			//u64 oldCapacity = capacity;
			capacity = newCapacity;
			auto desc = getDesc();
			RefCntAutoPtr<dl::IBuffer> newBuffer;
			Dev->CreateBuffer(desc, nullptr, &newBuffer);
			if (size > 0)
			{
				Imm->CopyBuffer(bufferHandle, 0, dl::RESOURCE_STATE_TRANSITION_MODE_TRANSITION,
					newBuffer, 0, size * sizeof(T), dl::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
			}
			bufferHandle = newBuffer;
		}

	private:
		RefCntAutoPtr<dl::IBuffer> bufferHandle;
		u64 size = 0;
		u64 capacity = 0;
		std::string name;
		dl::BIND_FLAGS bind;

	};

	template<typename T>
	class UniformBuffer
	{
	public:
		UniformBuffer() = default;
		UniformBuffer(const std::string& name, dl::USAGE usage = dl::USAGE_DYNAMIC)
		{
			dl::BufferDesc bdesc;
			bdesc.Name = name.c_str();
			bdesc.Usage = dl::USAGE_DYNAMIC;
			bdesc.BindFlags = dl::BIND_UNIFORM_BUFFER;
			bdesc.Size = sizeof(T);
			bdesc.CPUAccessFlags = usage == dl::USAGE_DYNAMIC ? dl::CPU_ACCESS_WRITE : dl::CPU_ACCESS_NONE;
			Dev->CreateBuffer(bdesc, nullptr, &bufferHandle);
		}

		UniformBuffer(const std::string& name, dl::USAGE usage, const T& t)
		{
			dl::BufferDesc bdesc;
			bdesc.Name = name.c_str();
			bdesc.Usage = dl::USAGE_DYNAMIC;
			bdesc.BindFlags = dl::BIND_UNIFORM_BUFFER;
			bdesc.Size = sizeof(T);
			bdesc.CPUAccessFlags = usage == dl::USAGE_DYNAMIC ? dl::CPU_ACCESS_WRITE : dl::CPU_ACCESS_NONE;
			Dev->CreateBuffer(bdesc, &t, &bufferHandle);
		}

		dl::MapHelper<T> Map()
		{
			return dl::MapHelper<T>(Imm, bufferHandle, dl::MAP_WRITE, dl::MAP_FLAG_DISCARD);
		}

		dl::IBuffer* GetBuffer() { return bufferHandle.RawPtr(); }

	private:
		RefCntAutoPtr<dl::IBuffer> bufferHandle;

	};
}
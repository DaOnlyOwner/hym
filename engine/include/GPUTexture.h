#pragma once
#include "Texture.h"
#include "Definitions.h"

namespace Hym
{
	enum class TextureType
	{
		Albedo, Normal, MetalRough, Emissive
	};

	struct Texture
	{
		RefCntAutoPtr<dl::ITexture> handle;
		u64 offset;
		TextureType textureType;
	};
}
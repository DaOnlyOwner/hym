#pragma once
#include "RenderDevice.h"
#include "DeviceContext.h"
#include "Definitions.h"
#include "SwapChain.h"
#include <vector>

namespace Hym
{
	extern RefCntAutoPtr<dl::IRenderDevice> Dev;
	extern std::vector<RefCntAutoPtr<dl::IDeviceContext>> DeferredCtxts;
	extern RefCntAutoPtr<dl::IDeviceContext> Imm;
	extern RefCntAutoPtr<dl::ISwapChain> SwapChain;
	extern RefCntAutoPtr<dl::IShaderSourceInputStreamFactory> ShaderStream;
}
#pragma once

#ifdef D3D12_SUPPORTED
#include "EngineFactoryD3D12.h"
#endif

#if VULKAN_SUPPORTED
#include <EngineFactoryVk.h>
#endif

#include "GLFW/glfw3.h"
#include "TimeConversion.h"
#include <string>
#include "Definitions.h"
#include <memory>

#include "imgui.h"
#include "ImGuiImplDiligent.hpp"
#include "ImGuiUtils.hpp"

namespace Hym
{
	struct InitInfo
	{
		enum class RenderBackend
		{
#if D3D12_SUPPORTED
			D3D12,
#endif

#if VULKAN_SUPPORTED
			Vulkan,
#endif
#if !D3D12_SUPPORTED && !VULKAN_SUPPORTED
			NoBackendSupported
#endif
		};
		bool enableDebugLayers = true;
		RenderBackend backend = RenderBackend::Vulkan;
		int width=800, height=600;
		std::string title;

	};

	void resizeCallback(GLFWwindow* window, int width, int height);


	class App
	{
	public:
		App(const InitInfo& init);
		int Run();
		virtual void Update(Time deltaTime) = 0;
		virtual void Shutdown();
		virtual void Resize(int w, int h){};
		virtual ~App() = default;

	private:
		void initDiligent(const InitInfo& info);
		
		dl::NativeWindow diligentWindow;
	protected:
		double GetMousePosX() const;
		double GetMousePosY() const;

		GLFWwindow* window;
		std::unique_ptr<dl::ImGuiImplDiligent> imguiImpl;
	};

}
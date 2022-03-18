#include "App.h"
#if PLATFORM_WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#endif
#include <GLFW/glfw3native.h>
#include <chrono>
#include "Debug.h"
#include "GlobalState.h"
#include <thread>
#include <algorithm>


void callback(Diligent::DEBUG_MESSAGE_SEVERITY Severity,
	const char* Message,
	const char* Function,
	const char* File,
	int                         Line)
{
	if (Severity == Diligent::DEBUG_MESSAGE_SEVERITY::DEBUG_MESSAGE_SEVERITY_FATAL_ERROR)
	{
		HYM_ERROR("{}", Message);
	}

	else if (Severity == Diligent::DEBUG_MESSAGE_SEVERITY::DEBUG_MESSAGE_SEVERITY_ERROR)
	{
		HYM_NON_FATAL_ERROR("{}", Message);
	}

	else if (Severity == Diligent::DEBUG_MESSAGE_SEVERITY::DEBUG_MESSAGE_SEVERITY_INFO)
	{
		HYM_INFO("{}", Message);
	}

	else if (Severity == Diligent::DEBUG_MESSAGE_SEVERITY::DEBUG_MESSAGE_SEVERITY_WARNING)
	{
		HYM_WARN("{}", Message);
	}
}

Hym::App::App(const InitInfo& init)
{
	bool succ = glfwInit();
	if (!succ)
	{
		HYM_ERROR("Couldn't initialize glfw");
	}
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	window = glfwCreateWindow(init.width, init.height, init.title.c_str(), NULL, NULL);
	if (!window)
	{
		HYM_ERROR("Couldn't create window");
	}
	initDiligent(init);
	glfwSetWindowUserPointer(window, this);
	glfwSetFramebufferSizeCallback(window, resizeCallback);
}

int Hym::App::Run()
{
	Time delta = 1;
	while (!glfwWindowShouldClose(window))
	{
		auto now = std::chrono::high_resolution_clock::now();
		glfwPollEvents();
		Update(delta);
		glfwSwapBuffers(window);
		auto then = std::chrono::high_resolution_clock::now();
		delta = Time(std::chrono::duration_cast<std::chrono::nanoseconds>(then - now).count());
	}
	glfwTerminate();
	Shutdown();
	return 0;
}

void Hym::App::Shutdown()
{
}

void Hym::App::initDiligent(const InitInfo& info)
{
	// Allocate at least two deferred contexts
	//dl::SetDebugMessageCallback(callback);

	DeferredCtxts.resize(std::max(std::thread::hardware_concurrency() - 1, 1u));
	std::vector<dl::IDeviceContext*> ctxts;
	ctxts.reserve(DeferredCtxts.size() + 1);
	ctxts.push_back(Imm);
	for (int i = 0; i < DeferredCtxts.size(); i++)
	{
		ctxts.push_back(DeferredCtxts[i]);
	}

	dl::SwapChainDesc sdesc;
	void* window_handle;
#if PLATFORM_WIN32
	window_handle = glfwGetWin32Window(window);
#elif PLATFORM_LINUX
	window_handle = glfwGetX11Window(window);
#else 
static_assert(false, "OSX is not supported");
#endif
	diligentWindow = dl::NativeWindow(window_handle);
	switch (info.backend)
	{
	case InitInfo::RenderBackend::Vulkan:
	{
		auto getVk = dl::LoadGraphicsEngineVk();
		dl::EngineVkCreateInfo vkCi;
		vkCi.EnableValidation = info.enableDebugLayers;
		vkCi.Features.RayTracing = dl::DEVICE_FEATURE_STATE_ENABLED;
		vkCi.SetValidationLevel(dl::VALIDATION_LEVEL_2);
		vkCi.NumDeferredContexts = DeferredCtxts.size();
		
		
		//vkCi.NumImmediateContexts = 1;
		auto* vkFactory = getVk();
		vkFactory->SetMessageCallback(callback);
		vkFactory->CreateDeviceAndContextsVk(vkCi, &Dev, ctxts.data());
		vkFactory->CreateSwapChainVk(Dev.RawPtr(), ctxts[0], sdesc, diligentWindow, &SwapChain);
		vkFactory->CreateDefaultShaderSourceStreamFactory(SHADER_RES, &ShaderStream);
	}
		break;
		
	case InitInfo::RenderBackend::D3D12:
	{
		auto getD3 = dl::LoadGraphicsEngineD3D12();
		dl::EngineD3D12CreateInfo D3Ci;
		D3Ci.EnableValidation = info.enableDebugLayers;
		D3Ci.Features.RayTracing = dl::DEVICE_FEATURE_STATE_ENABLED;
		D3Ci.SetValidationLevel(dl::VALIDATION_LEVEL_2);
		D3Ci.NumDeferredContexts = DeferredCtxts.size();
		//D3Ci.NumImmediateContexts = 1;	
		auto* d3Factory = getD3();
		d3Factory->SetMessageCallback(callback);
		d3Factory->CreateDeviceAndContextsD3D12(D3Ci, &Dev, ctxts.data());
		d3Factory->CreateSwapChainD3D12(Dev.RawPtr(), ctxts[0], sdesc, dl::FullScreenModeDesc{}, diligentWindow, &SwapChain);
		d3Factory->CreateDefaultShaderSourceStreamFactory(SHADER_RES, &ShaderStream);
	}
		break;
	default:
		break;		
	}

	Imm = ctxts[0];
	for (int i = 1; i < ctxts.size(); i++)
	{
		DeferredCtxts[i-1] = ctxts[i];
	}
	//Renderer::Inst().Init(device, imm, swapChain,streamFactory);
}

double Hym::App::GetMousePosX() const {
	double px, py;
	glfwGetCursorPos(window, &px, &py);
	return px;
}

double Hym::App::GetMousePosY() const
{
	double px, py;
	glfwGetCursorPos(window, &px, &py);
	return py;
}

void Hym::resizeCallback(GLFWwindow* window, int width, int height)
{
	App* a = (App*)glfwGetWindowUserPointer(window);
	a->Resize(width, height);
}

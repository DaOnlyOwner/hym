#include "GlobalState.h"

std::vector<Hym::dl::RefCntAutoPtr<Hym::dl::IDeviceContext>> Hym::DeferredCtxts;
Hym::dl::RefCntAutoPtr<Hym::dl::IDeviceContext> Hym::Imm;
Hym::dl::RefCntAutoPtr<Hym::dl::ISwapChain> Hym::SwapChain;
Hym::dl::RefCntAutoPtr<Hym::dl::IRenderDevice> Hym::Dev;
Hym::RefCntAutoPtr<Hym::dl::IShaderSourceInputStreamFactory> Hym::ShaderStream;
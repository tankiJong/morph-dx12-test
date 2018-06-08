#pragma once
#include "Engine/Core/common.hpp"
#include "Engine/Application/Window.hpp"
#include <d3d12.h>

class Dx12Device;
class Dx12Instance;
struct IDXGISwapChain3;

class Dx12Output {
  friend class Dx12Device;
  friend class Dx12CommandList;
public:
  static constexpr uint FRAME_COUNT = 2;
  void syncFrameIndex();
  ID3D12Resource* getCurrentBackBuffer();
  uint getFrameIndex();

  void present();
  bool quit();
  HWND mWindow = nullptr;
protected:
  Dx12Output(Dx12Device* device);


  S<IDXGISwapChain3> mSwapChain = nullptr;
  S<ID3D12Resource> mRenderTargets[FRAME_COUNT];
  S<Dx12Device> mDevice;
  uint mFrameIndex;
  D3D12_VIEWPORT mViewport;
  D3D12_RECT mScissorRect;
};

#pragma once
#include "Engine/Core/common.hpp"
#include "Game/dx12/Dx12Output.hpp"

class Dx12Device;
struct ID3D12Debug;

struct IDXGIFactory4;

class Dx12Instance {
  friend Dx12Output;
public:
  S<Dx12Device> createDevice();
protected:
  ID3D12Debug* mDebugLayer = nullptr;
  IDXGIFactory4* mGIFractory = nullptr;
};

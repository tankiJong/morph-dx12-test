#pragma once
#include "Engine/Core/common.hpp"
#include "Game/dx12/Dx12CommandList.hpp"
#include <mutex>

struct ID3D12Device;
class Dx12Output;
struct D3D12_CPU_DESCRIPTOR_HANDLE;
struct ID3D12CommandQueue;
class RootSignature;
class Dx12Device {
  friend class Dx12Instance;
  friend class Dx12Output;
  friend class Dx12CommandList;
public:
  S<Dx12CommandList> getImmediateCommandList();
  S<Dx12Output> getOutput();
  u64 dispatchCommandList(Dx12CommandList& context);
  void* nativeDevice();
  
  bool isFenceComplete(u64 fenceValue);
  void waitForFence(u64 fenceValue);
  u64 increaseFence();
  D3D12_CPU_DESCRIPTOR_HANDLE getRtvHandle();
protected:
  Dx12Device(Dx12Instance* instance, ID3D12Device* device);
  void initOutput();
  S<ID3D12Device> mDevice = nullptr;
  S<ID3D12CommandQueue> mCommandQueue = nullptr;
  S<ID3D12DescriptorHeap> mRtvHeap = nullptr;
  //S<ID3D12DescriptorHeap> mSrvHeap = nullptr;

  S<Dx12Instance> const mInstance = nullptr;
  S<Dx12Output> mOutput = nullptr;
  S<Dx12CommandList> mImmediateContext = nullptr;

  uint mRtvDexcriptorSize = 0;
  S<ID3D12Fence> mFence = nullptr;
  void* mFenceEvent = nullptr;
  std::mutex mEventMutex;
  u64 mNextFenceValue;
  u64 mLastCompletedFenceValue;
};

#include "Dx12Device.hpp"
#include "d3d12.h"
#include "dx12util.hpp"
#include "Game/dx12/Dx12Output.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Debug/ErrorWarningAssert.hpp"

S<Dx12CommandList> Dx12Device::getImmediateCommandList() {
  if(mImmediateContext == nullptr) {
    mImmediateContext.reset(new Dx12CommandList(S<Dx12Device>(this)));
  }

  return mImmediateContext;
}

S<Dx12Output> Dx12Device::getOutput() {
  return mOutput;
}

u64 Dx12Device::dispatchCommandList(Dx12CommandList& context) {

  std::lock_guard<std::mutex> mGuard(mEventMutex);
  HRESULT r = context.mCommandList->Close();
  //EXPECT_HR_SUCCESSED(context.mCommandList->Close());

  ID3D12CommandList* ppCommandLists[] = { context.mCommandList.get() };
  mCommandQueue->ExecuteCommandLists(countof(ppCommandLists), ppCommandLists);

  mCommandQueue->Signal(mFence.get(), mNextFenceValue);

  return mNextFenceValue++;
}

void* Dx12Device::nativeDevice() {
  return (void*)mDevice.get();
}

bool Dx12Device::isFenceComplete(u64 fenceValue) {
  if (fenceValue > mLastCompletedFenceValue)
    mLastCompletedFenceValue = std::max<u64>(mLastCompletedFenceValue, mFence->GetCompletedValue());

  return fenceValue <= mLastCompletedFenceValue;
}

void Dx12Device::waitForFence(u64 fenceValue) {
  if (isFenceComplete(fenceValue))
    return;

  // TODO:  Think about how this might affect a multi-threaded situation.  Suppose thread A
  // wants to wait for fence 100, then thread B comes along and wants to wait for 99.  If
  // the fence can only have one event set on completion, then thread B has to wait for 
  // 100 before it knows 99 is ready.  Maybe insert sequential events?
  {
    std::lock_guard<std::mutex> lock(mEventMutex);

    mFence->SetEventOnCompletion(fenceValue, mFenceEvent);
    WaitForSingleObject(mFenceEvent, INFINITE);
    mLastCompletedFenceValue = fenceValue;
  }
}

u64 Dx12Device::increaseFence() {
  std::lock_guard<std::mutex> lock(mEventMutex);

  mCommandQueue->Signal(mFence.get(), mNextFenceValue);
  return mNextFenceValue;
}

D3D12_CPU_DESCRIPTOR_HANDLE Dx12Device::getRtvHandle() {
  D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle(mRtvHeap->GetCPUDescriptorHandleForHeapStart());
  rtvHandle.ptr += mOutput->getFrameIndex() * mRtvDexcriptorSize;
  return rtvHandle;
}

Dx12Device::Dx12Device(Dx12Instance* instance, ID3D12Device* device)
  : mInstance(instance)
  , mDevice(device)
  , mNextFenceValue((u64)D3D12_COMMAND_LIST_TYPE_DIRECT << 56 | 1)    // TODO: should replace with param type later
  , mLastCompletedFenceValue((uint64_t)D3D12_COMMAND_LIST_TYPE_DIRECT << 56) {
  D3D12_COMMAND_QUEUE_DESC queueDesc = {};
  queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
  queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT; // https://msdn.microsoft.com/en-us/library/dn770348(v=vs.85).aspx

  ID3D12CommandQueue* cq = mCommandQueue.get();

  breakOnFail(mDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&cq)));

  mCommandQueue.reset(cq);

  /****************************************
  *
  * create descriptor heaps
  * https://msdn.microsoft.com/en-us/library/windows/desktop/dn899110(v=vs.85).aspx
  * A descriptor heap is a collection of contiguous allocations of descriptors, one allocation for every descriptor.
  *
  ****************************************/

  // Describe and create a render target view (RTV) descriptor heap.

  ID3D12DescriptorHeap* rtvHeap = mRtvHeap.get();

  D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
  rtvHeapDesc.NumDescriptors = Dx12Output::FRAME_COUNT;
  rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
  rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
  breakOnFail(mDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvHeap)));

  mRtvHeap.reset(rtvHeap);

  mRtvDexcriptorSize = mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

  //ID3D12DescriptorHeap* srvHeap = mSrvHeap.get();

  //// Describe and create a shader resource view (SRV) heap for the texture.
  //D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
  //srvHeapDesc.NumDescriptors = 0;
  //srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
  //srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
  //breakOnFail(mDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&srvHeap)));

  //mSrvHeap.reset(srvHeap);

  initOutput();

  ID3D12Fence* fence = mFence.get();
  breakOnFail(mDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
  mFence.reset(fence);

  mFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
  if (mFenceEvent == nullptr) {
    breakOnFail(HRESULT_FROM_WIN32(GetLastError()));
  }

}

void Dx12Device::initOutput() {
  Dx12Output* output =  new Dx12Output(this);
  mOutput.reset(output);
}

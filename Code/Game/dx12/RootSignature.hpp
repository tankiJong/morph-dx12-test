#pragma once
#include "Engine/Core/common.hpp"
#include <d3d12.h>
#include <vector>

class Dx12Device;

struct RootParamater {

protected:
  D3D12_ROOT_PARAMETER mRootParam;
};



class RootSignature {
public:
  void finalize(const std::string& name, D3D12_ROOT_SIGNATURE_FLAGS flags = D3D12_ROOT_SIGNATURE_FLAG_NONE);
  void finalize(const std::wstring& name, D3D12_ROOT_SIGNATURE_FLAGS flags = D3D12_ROOT_SIGNATURE_FLAG_NONE);
  RootSignature(S<Dx12Device> device);

  void* native();
  const void* native() const;
protected:
  bool mFinalized = false;
  ID3D12RootSignature* mSignature = nullptr;
  std::vector<RootParamater> mParamArray;
  std::vector<D3D12_STATIC_SAMPLER_DESC> mSamplerArray;
  uint mDescriptorTableBitmap = 0;
  uint mSamplerTableBitmap = 0;
  // from mini engine
  uint32_t m_DescriptorTableSize[16];		// Non-sampler descriptor tables need to know their descriptor count
  S<Dx12Device> mDevice;
};


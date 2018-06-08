#include "RootSignature.hpp"
#include "Engine/Debug/ErrorWarningAssert.hpp"
#include "Game/dx12/dx12util.hpp"
#include "Game/dx12/Dx12Device.hpp"

void RootSignature::finalize(const std::string& name, D3D12_ROOT_SIGNATURE_FLAGS flags) {
  finalize(make_wstring(name), flags);
}

void RootSignature::finalize(const std::wstring& name, D3D12_ROOT_SIGNATURE_FLAGS flags) {
  if (mFinalized) return;

  D3D12_ROOT_SIGNATURE_DESC rootDesc;
  rootDesc.NumParameters = mParamArray.size();
  rootDesc.pParameters = (D3D12_ROOT_PARAMETER*)mParamArray.data();
  rootDesc.pStaticSamplers = mSamplerArray.data();
  rootDesc.NumStaticSamplers = mSamplerArray.size();
  rootDesc.Flags = flags;

  mDescriptorTableBitmap = 0;
  mSamplerTableBitmap = 0;

  for (uint param = 0; param < mParamArray.size(); ++param) {
    const D3D12_ROOT_PARAMETER& rootPararm = rootDesc.pParameters[param];
    m_DescriptorTableSize[param] = 0;

    if (rootPararm.ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE) {
      EXPECTS(rootPararm.DescriptorTable.pDescriptorRanges != nullptr);
      if (rootPararm.DescriptorTable.pDescriptorRanges->RangeType == D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER)
        mSamplerTableBitmap |= (1 << param);
      else mDescriptorTableBitmap |= (1 << param);

      for (uint tableRange = 0; tableRange < rootPararm.DescriptorTable.NumDescriptorRanges; ++tableRange) {
        m_DescriptorTableSize[param] += rootPararm.DescriptorTable.pDescriptorRanges[tableRange].NumDescriptors;
      }
    }

  }

  ID3DBlob* signature;
  ID3DBlob* error;
  EXPECT_HR_SUCCESSED(D3D12SerializeRootSignature(&rootDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));

  EXPECT_HR_SUCCESSED(((ID3D12Device*)mDevice->nativeDevice())->CreateRootSignature(1, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&mSignature)));

  mSignature->SetName(name.c_str());

  mFinalized = true;
}

RootSignature::RootSignature(S<Dx12Device> device): mDevice(device) {}

void* RootSignature::native() {
  return mSignature;
}

const void* RootSignature::native() const {
  return mSignature;
}

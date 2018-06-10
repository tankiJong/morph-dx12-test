#pragma once
#include "Engine/Core/common.hpp"
#include <d3d12.h>
#include "Engine/Renderer/type.h"

class RootSignature;
class VertexLayout;
class Dx12Device;
class ID3D12PipelineState;

class PipelineState {
public:
  PipelineState(S<Dx12Device> device);;

  void setRootSignature(S<RootSignature> rootSignature);
  S<RootSignature> getRootSignature();
  void setInputLayout(const VertexLayout& layout);
  void setRenderState(const render_state& state);
  void finalize();
  void reset();

  void* native();
protected:
  S<Dx12Device> mDevice;
  S<RootSignature> mRootSignature = nullptr;
  ID3D12PipelineState* mPipelineState = nullptr;
  D3D12_GRAPHICS_PIPELINE_STATE_DESC mPipelinestateDesc;
  S<D3D12_INPUT_ELEMENT_DESC> mInputLayouts;
};

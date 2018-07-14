#include "PipelineState.hpp"
#include "Engine/Graphics/RHI/VertexLayout.hpp"
#include <d3d12.h>
#include "Game/dx12/dx12util.hpp"
#include "Game/dx12/RootSignature.hpp"
#include "Engine/Debug/ErrorWarningAssert.hpp"
#include "Game/dx12/Dx12Device.hpp"
#include "Engine/File/Path.hpp"
#include "Engine/File/Utils.hpp"
#include <d3dcompiler.h>

PipelineState::PipelineState(S<Dx12Device> device): mDevice(device) {
  reset();
}

void PipelineState::setRootSignature(S<RootSignature> rootSignature) {
  mRootSignature = rootSignature;
}

void PipelineState::setInputLayout(const VertexLayout& layout) {
  // TODO: cache up input layout

  D3D12_INPUT_LAYOUT_DESC inputDesc;
  auto attrs = layout.attributes();

  D3D12_INPUT_ELEMENT_DESC* eles = new D3D12_INPUT_ELEMENT_DESC[attrs.size()];

  for(uint i = 0; i < attrs.size(); ++i) {
    D3D12_INPUT_ELEMENT_DESC& ele = eles[i];
    const VertexAttribute& attr = attrs[i];
    ele.SemanticName = attr.name.c_str();
    ele.SemanticIndex = 0;
    ele.Format = toDXGIFormat(attr.type, attr.count, attr.isNormalized);
    ele.InputSlot = attr.streamIndex;
    ele.AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
    ele.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
    ele.InstanceDataStepRate = 0;
  }


  mPipelinestateDesc.InputLayout = { eles, (uint)attrs.size() };
  mInputLayouts.reset(eles);
}

void PipelineState::finalize() {
  mPipelinestateDesc.pRootSignature = (ID3D12RootSignature*)mRootSignature->native();
  ENSURES(mPipelinestateDesc.pRootSignature != nullptr);

  ((ID3D12Device*)mDevice->nativeDevice())->CreateGraphicsPipelineState(&mPipelinestateDesc, IID_PPV_ARGS(&mPipelineState));
}

void PipelineState::reset() {

  ID3DBlob* vertexShader = nullptr;
  ID3DBlob* pixelShader = nullptr;

#if defined(_DEBUG)
  // Enable better shader debugging with the graphics debugging tools.
  UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
  UINT compileFlags = 0;
#endif

  fs::path shaderPath = fs::absolute("shaders.hlsl");
  breakOnFail(D3DCompileFromFile(shaderPath.c_str(), nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &vertexShader, nullptr));
  breakOnFail(D3DCompileFromFile(shaderPath.c_str(), nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &pixelShader, nullptr));
  // create vertex input layout
  // setInputLayout(*VertexLayout::For<vertex_pcu_t>());

  // Describe and create the graphics pipeline state object (PSO).
  D3D12_SHADER_BYTECODE VS, PS;
  VS.pShaderBytecode = vertexShader->GetBufferPointer();
  VS.BytecodeLength = vertexShader->GetBufferSize();
  PS.pShaderBytecode = pixelShader->GetBufferPointer();
  PS.BytecodeLength = pixelShader->GetBufferSize();

  // default rasterizer
  D3D12_RASTERIZER_DESC rasterizerDesc;
  rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
  rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
  rasterizerDesc.FrontCounterClockwise = FALSE;
  rasterizerDesc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
  rasterizerDesc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
  rasterizerDesc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
  rasterizerDesc.DepthClipEnable = TRUE;
  rasterizerDesc.MultisampleEnable = FALSE;
  rasterizerDesc.AntialiasedLineEnable = FALSE;
  rasterizerDesc.ForcedSampleCount = 0;
  rasterizerDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

  // default Blend
  D3D12_BLEND_DESC blendDesc;
  blendDesc.AlphaToCoverageEnable = FALSE;
  blendDesc.IndependentBlendEnable = FALSE;
  const D3D12_RENDER_TARGET_BLEND_DESC defaultRenderTargetBlendDesc =
  {
    FALSE,FALSE,
    D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
    D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
    D3D12_LOGIC_OP_NOOP,
    D3D12_COLOR_WRITE_ENABLE_ALL,
  };

  for (UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
    blendDesc.RenderTarget[i] = defaultRenderTargetBlendDesc;

  mPipelinestateDesc.VS = VS;
  mPipelinestateDesc.PS = PS;
  mPipelinestateDesc.RasterizerState = rasterizerDesc;
  mPipelinestateDesc.BlendState = blendDesc;
  mPipelinestateDesc.DepthStencilState.DepthEnable = FALSE;
  mPipelinestateDesc.DepthStencilState.StencilEnable = FALSE;
  mPipelinestateDesc.SampleMask = UINT_MAX;
  mPipelinestateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
  mPipelinestateDesc.NumRenderTargets = 1;
  mPipelinestateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
  mPipelinestateDesc.SampleDesc.Count = 1;

}

void* PipelineState::native() {
  return mPipelineState;
}

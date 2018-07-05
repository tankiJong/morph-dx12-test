#define WIN32_LEAN_AND_MEAN		// Always #define this before #including <windows.h>
#include <Windows.h>


#include "Engine/Core/type.h"
#include "Engine/Debug/ErrorWarningAssert.hpp"
#include "Engine/File/Utils.hpp"
#include "Engine/Graphics/RHI/RHIDevice.hpp"
#include "Engine/Graphics/RHI/RootSignature.hpp"
#include "Engine/Graphics/RHI/PipelineState.hpp"
#include "Engine/Graphics/RHI/RHITexture.hpp"
#include "Engine/Graphics/RHI/ResourceView.hpp"
#include "Engine/Core/Time/Clock.hpp"
#include "Engine/Graphics/RHI/Texture.hpp"
#define UNUSED(x) (void)x

// -------------------------  constant ------------------------------
constexpr uint frameCount = 2;
// ------------------------------------------------------------------

constexpr uint texWidth = 256;
constexpr uint texHeight = 256;
// Generate a simple black and white checkerboard texture.
std::vector<UINT8> GenerateTextureData() {
  const UINT rowPitch = texWidth * texHeight;
  const UINT cellPitch = rowPitch >> 3;		// The width of a cell in the checkboard texture.
  const UINT cellHeight = texHeight >> 3;	// The height of a cell in the checkerboard texture.
  const UINT textureSize = rowPitch * 256;

  std::vector<UINT8> data(textureSize);
  UINT8* pData = &data[0];

  for (UINT n = 0; n < textureSize; n += 4) {
    UINT x = n % rowPitch;
    UINT y = n / rowPitch;
    UINT i = x / cellPitch;
    UINT j = y / cellHeight;

    if (i % 2 == j % 2) {
      pData[n] = 0x00;		// R
      pData[n + 1] = 0x00;	// G
      pData[n + 2] = 0x00;	// B
      pData[n + 3] = 0xff;	// A
    } else {
      pData[n] = 0xff;		// R
      pData[n + 1] = 0xff;	// G
      pData[n + 2] = 0xff;	// B
      pData[n + 3] = 0xff;	// A
    }
  }

  return data;
}

extern bool gQuit;
uint width = 1280;
uint height = 720;

S<RHIDevice> mDevice;
S<RHIContext> mContext;
PipelineState::sptr_t pipelineState;
RootSignature::sptr_t rootSig;

RHIBuffer::sptr_t vbo[3];
RHIBuffer::sptr_t cTint;
Texture2::sptr_t texture;
ConstantBufferView::sptr_t tintCbv;
ShaderResourceView::sptr_t texSrv;
DescriptorSet::sptr_t descriptorSet;
void Initialize() {
  mDevice = RHIDevice::create();

  mContext = mDevice->defaultRenderContext();
  mContext->initDefaultRenderTarget();
  {
    RootSignature::Desc desc;
    RootSignature::desc_set_layout_t layout;
    layout.addRange(DescriptorSet::Type::Cbv, 0, 1, 0);
    layout.addRange(DescriptorSet::Type::TextureSrv, 0, 1, 0);
    descriptorSet = DescriptorSet::create(mDevice->gpuDescriptorPool(), layout);
    desc.addDescriptorSet(layout);
    rootSig = RootSignature::create(desc);
  }
  {
    PipelineState::Desc desc;
    desc.setRootSignature(rootSig);
    desc.setPrimTye(PipelineState::PrimitiveType::Triangle);
    desc.setVertexLayout(VertexLayout::For<vertex_pcu_t>());
    pipelineState = PipelineState::create(desc);

    mContext->setPipelineState(pipelineState);
  }


  // mContext->flush();

  constexpr uint numVerts = 6;

  vec3 pos[numVerts] = {
    { 0.0f, 0.25f, 0.0f },
  { 0.25f, -0.25f, 0.0f },
  { 0.f, -0.25f, 0.0f },
  { 0.0f, 0.25f, 0.0f },
  { -0.25f, -0.25f, 0.0f },
  { 0.f, -0.25f, 0.0f }
  };
  Rgba color[numVerts] = {
    Rgba::red,
    Rgba::green,
    Rgba::blue,
    Rgba::red,
    Rgba::green,
    Rgba::blue,
  };

  vec2 uvs[numVerts] = {
    vec2::right,
    vec2::zero,
    vec2::top,
    vec2::top,
    vec2::zero,
    vec2::right,
  };

  byte_t* verts[3] = {
    (byte_t*)pos,
    (byte_t*)color,
    (byte_t*)uvs,
  };

  vbo[0] =
    RHIBuffer::create(sizeof(vec3) * numVerts, RHIResource::BindingFlag::VertexBuffer, RHIBuffer::CPUAccess::Write, pos);
  vbo[1] =
  RHIBuffer::create(sizeof(Rgba) * numVerts, RHIResource::BindingFlag::VertexBuffer, RHIBuffer::CPUAccess::Write, color);
  vbo[2] =
  RHIBuffer::create(sizeof(vec2) * numVerts, RHIResource::BindingFlag::VertexBuffer, RHIBuffer::CPUAccess::Write, uvs);

  vec4 tint = vec4(0.5f, .5f, .5f, 1.f);
  cTint = RHIBuffer::create(sizeof(vec4), RHIResource::BindingFlag::ConstantBuffer, RHIBuffer::CPUAccess::Write, &tint);
  //
  std::vector<UINT8> data = GenerateTextureData();
  texture = Texture2::create(texWidth, texHeight, RHIResource::BindingFlag::ShaderResource, data.data(), data.size());

  tintCbv = ConstantBufferView::create(cTint);
  texSrv = ShaderResourceView::create(texture, 0, 1, 0, 1);
  descriptorSet->setCbv(0, 0, *tintCbv);
  descriptorSet->setSrv(0, 1, *texSrv);
  mContext->resourceBarrier(texture.get(), RHIResource::State::ShaderResource);
  mContext->flush();
}

void RunFrame() {
  static float a = 1.f;
  a += 1.f;
  a = fmod(a, 360.f);
  static uint frame   = 10;
  static uint current = 0;
  mContext->beforeFrame();
  vec4 color = Hsl(a, 1.f, .5f).normalized();
  mContext->updateBuffer(cTint.get(), &color, 0, sizeof(vec4));
  mContext->setPipelineState(pipelineState);
  mContext->bindDescriptorHeap();
  descriptorSet->bindForGraphics(*mContext, *rootSig, 0);

  uint stride[3] = {sizeof(vec3), sizeof(Rgba), sizeof(vec2)};
  for(int i = 0; i < 3; i++) {
    mContext->setVertexBuffer(vbo[i], stride[i], i);
  }

  mContext->draw(0, 6);
  // mContext->draw(3, 3);
  mContext->afterFrame();
  mDevice->present();
}

//-----------------------------------------------------------------------------------------------
int __stdcall WinMain(HINSTANCE, HINSTANCE, LPSTR commandLineString, int) {
  UNUSED(commandLineString);
  Initialize();

  // Program main loop; keep running frames until it's time to quit
  while (!gQuit) {
    MSG message;
    while (PeekMessage(&message, mDevice->mWindow, 0, 0, PM_REMOVE)) {
      TranslateMessage(&message);
      DispatchMessage(&message);
    }
    RunFrame();
  }

  //Shutdown();
  return 0;
}


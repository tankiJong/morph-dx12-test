#define WIN32_LEAN_AND_MEAN		// Always #define this before #including <windows.h>
#include <Windows.h>


#include "Engine/Core/type.h"
#include "Engine/File/Utils.hpp"
#include "Engine/Graphics/RHI/RHIDevice.hpp"
#include "Engine/Graphics/RHI/RootSignature.hpp"
#include "Engine/Graphics/RHI/PipelineState.hpp"
#include "Engine/Graphics/RHI/RHITexture.hpp"
#include "Engine/Graphics/RHI/ResourceView.hpp"
#include "Engine/Core/Time/Clock.hpp"
#include "Engine/Graphics/RHI/Texture.hpp"
#include "Engine/Graphics/Model/Vertex.hpp"
#include "Engine/Graphics/Camera.hpp"
#include "Engine/Core/Engine.hpp"
#include "Engine/Input/Input.hpp"
#include "Engine/Application/Window.hpp"
#include "Engine/Graphics/Model/Mesher.hpp"
#define UNUSED(x) (void)x

static bool gQuit = false;
void CALLBACK windowProc(uint wmMessageCode, size_t /*wParam*/, size_t lParam) {
  UNUSED(lParam);
  switch (wmMessageCode) {
    // App close requested via "X" button, or right-click "Close Window" on task bar, or "Close" from system menu, or Alt-F4
    case WM_CLOSE:
    {
      gQuit = true;
      return; // "Consumes" this message (tells Windows "okay, we handled it")
    }
  }
}

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

S<RHIDevice> mDevice;
S<RHIContext> mContext;
PipelineState::sptr_t pipelineState;
RootSignature::sptr_t rootSig;

Camera* mCamera;
light_info_t mLight;
Transform mLightTransform;

RHIBuffer::sptr_t vbo[4];
RHIBuffer::sptr_t ibo;
RHIBuffer::sptr_t cVp;
RHIBuffer::sptr_t cLight;
Texture2::sptr_t texture;
ConstantBufferView::sptr_t vpCbv;
ConstantBufferView::sptr_t lightCbv;
ShaderResourceView::sptr_t texSrv;
DescriptorSet::sptr_t descriptorSet;
FrameBuffer* frameBuffer;



uint elementCount = 0; 
uint numVerts = 0;
void Initialize() {
  Window::Get()->addWinMessageHandler(windowProc);
  mCamera = new Camera();
  mCamera->transfrom().localPosition() = { -1, -1, -1 };
  mCamera->lookAt({ -1, -1, -1 }, vec3::zero);
  mCamera->setProjectionPrespective(30.f, 3.f*CLIENT_ASPECT, 3.f, .1f, 100.f);
  
  mDevice = RHIDevice::get();
  mContext = mDevice->defaultRenderContext();

  // main pass
    FrameBuffer::Desc fDesc;
    fDesc.defineColorTarget(0, TEXTURE_FORMAT_RGBA8);
    fDesc.defineDepthTarget(TEXTURE_FORMAT_D24S8);
    frameBuffer = new FrameBuffer(fDesc);
    {
      RootSignature::Desc desc;
      RootSignature::desc_set_layout_t layout;
      layout.addRange(DescriptorSet::Type::Cbv, 0, 2, 0);
      layout.addRange(DescriptorSet::Type::TextureSrv, 0, 1, 0);
      descriptorSet = DescriptorSet::create(mDevice->gpuDescriptorPool(), layout);
      desc.addDescriptorSet(layout);
      rootSig = RootSignature::create(desc);
    }
    {
      PipelineState::Desc desc;
      desc.setRootSignature(rootSig);
      desc.setPrimTye(PipelineState::PrimitiveType::Triangle);
      desc.setVertexLayout(VertexLayout::For<vertex_lit_t>());
      desc.setFboDesc(fDesc);
      pipelineState = PipelineState::create(desc);
      mContext->setPipelineState(pipelineState);
    }

  // mContext->initDefaultRenderTarget();


  // mContext->flush();

  Mesher ms;

  ms.begin(DRAW_TRIANGES);
  ms.quad(vec3::zero, vec3::right, vec3::forward, vec2(5.f));
  ms.sphere(vec3(0, 1, 0), 1.f, 20, 20);
  ms.end();

  vec3* pos = ms.mVertices.vertices().position;
  vec3* normals = ms.mVertices.vertices().normal;
  Rgba* color = ms.mVertices.vertices().color;
  vec2* uvs = ms.mVertices.vertices().uv;
  numVerts = ms.mVertices.count();
  elementCount = ms.mIndices.size();
  vbo[0] =
    RHIBuffer::create(sizeof(vec3) * numVerts, RHIResource::BindingFlag::VertexBuffer, RHIBuffer::CPUAccess::Write, pos);
  vbo[1] =
  RHIBuffer::create(sizeof(Rgba) * numVerts, RHIResource::BindingFlag::VertexBuffer, RHIBuffer::CPUAccess::Write, color);
  vbo[2] =
  RHIBuffer::create(sizeof(vec2) * numVerts, RHIResource::BindingFlag::VertexBuffer, RHIBuffer::CPUAccess::Write, uvs);
  vbo[3] =
    RHIBuffer::create(sizeof(vec3) * numVerts, RHIResource::BindingFlag::VertexBuffer, RHIBuffer::CPUAccess::Write, normals);

  ibo = RHIBuffer::create(sizeof(uint) * elementCount, RHIResource::BindingFlag::IndexBuffer,
                          RHIBuffer::CPUAccess::Write, ms.mIndices.data());

  camera_t cameraUbo = mCamera->ubo();
  cVp = RHIBuffer::create(sizeof(camera_t), RHIResource::BindingFlag::ConstantBuffer, RHIBuffer::CPUAccess::Write, &cameraUbo);

  light_info_t light;
  light.asSpotLight(mCamera->transfrom().position(), mCamera->transfrom().forward(), 5.f, 10.f, 1.f);
  cLight = RHIBuffer::create(sizeof(light_info_t), RHIResource::BindingFlag::ConstantBuffer, RHIBuffer::CPUAccess::Write,
                             &light);
  //
  std::vector<UINT8> data = GenerateTextureData();
  texture = Texture2::create(texWidth, texHeight, TEXTURE_FORMAT_RGBA8, RHIResource::BindingFlag::ShaderResource, data.data(), data.size());

  vpCbv = ConstantBufferView::create(cVp);
  lightCbv = ConstantBufferView::create(cLight);
  texSrv = ShaderResourceView::create(texture, 0, 1, 0, 1);
  descriptorSet->setCbv(0, 0, *vpCbv);
  descriptorSet->setCbv(0, 1, *lightCbv);
  descriptorSet->setSrv(1, 0, *texSrv);
  mContext->resourceBarrier(texture.get(), RHIResource::State::ShaderResource);
  // mContext->flush();
}

void onInput() {
  static float angle = 0.f;
  static float distance = 2.f;
  static float langle = 10.f;
  static float ldistance = 2.f;
  if(Input::Get().isKeyDown('W')) {
    distance -= 0.05f;
  }
  if (Input::Get().isKeyDown('S')) {
    distance += 0.05f;
  }
  if (Input::Get().isKeyDown(KEYBOARD_UP)) {
    ldistance -= 0.05f;
  }
  if (Input::Get().isKeyDown(KEYBOARD_DOWN)) {
    ldistance += 0.05f;
  }

  distance = std::max(.1f, distance);
  ldistance = std::max(.1f, ldistance);
  if(Input::Get().isKeyDown('A')) {
    angle -= 1.f;
  }
  if (Input::Get().isKeyDown('D')) {
    angle += 1.f;
  }
  if (Input::Get().isKeyDown(KEYBOARD_LEFT)) {
    langle -= 1.f;
  }
  if (Input::Get().isKeyDown(KEYBOARD_RIGHT)) {
    langle += 1.f;
  }
  vec3 position = fromSpherical(distance, angle, 30.f);
  mCamera->lookAt(position, vec3::zero);
  mLightTransform.setWorldTransform(mat44::lookAt(fromSpherical(ldistance, langle, 30.f), vec3::zero));
  mLight.asSpotLight(mLightTransform.position(), mLightTransform.forward(), 5.f, 10.f, 1.f);

};


void mainPass() {
  mContext->setFrameBuffer(*frameBuffer);
  camera_t cameraUbo = mCamera->ubo();
  // cTint->updateData(&color, 0, sizeof(vec4));
  cVp->updateData(&cameraUbo, 0, sizeof(camera_t));

  cLight->updateData(&mLight, 0, sizeof(light_info_t));
  mContext->setPipelineState(pipelineState);
  mContext->bindDescriptorHeap();

  descriptorSet->bindForGraphics(*mContext, *rootSig, 0);

  uint stride[] = { sizeof(vec3), sizeof(Rgba), sizeof(vec2), sizeof(vec3) };
  for (int i = 0; i < 4; i++) {
    mContext->setVertexBuffer(vbo[i], stride[i], i);
  }
  mContext->setIndexBuffer(ibo);

  mContext->drawIndexed(0, 0, elementCount);
  // mContext->draw(3, 3);
  // mContext->afterFrame();
}

void SSAO() {
  
}

void render() {
  mContext->beforeFrame();
  frameBuffer->setColorTarget(mDevice->backBuffer(), 0);
  frameBuffer->setDepthStencilTarget(mDevice->depthBuffer());
  mContext->clearRenderTarget(
    mDevice->backBuffer()->rtv(),
    Rgba(vec3{ 0.0f, 0.2f, 0.4f }));
  mContext->clearDepthStencilTarget(*mDevice->depthBuffer()->dsv());

  mainPass();
  mDevice->present();
}

void runFrame() {
  Input::Get().beforeFrame();
  onInput();
  render();
  Input::Get().afterFrame();

}

void Shutdown() {
  mDevice->cleanup();
}
//-----------------------------------------------------------------------------------------------
int __stdcall WinMain(HINSTANCE, HINSTANCE, LPSTR commandLineString, int) {
  UNUSED(commandLineString);
  Engine::Get();
  Initialize();

  // Program main loop; keep running frames until it's time to quit
  while (!gQuit) {
    MSG message;
    while (PeekMessage(&message, mDevice->mWindow, 0, 0, PM_REMOVE)) {
      TranslateMessage(&message);
      DispatchMessage(&message);
    }
    runFrame();
  }

  Shutdown();
  return 0;
}


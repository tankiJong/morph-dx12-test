#define WIN32_LEAN_AND_MEAN		// Always #define this before #including <windows.h>
#include <Windows.h>

#include "dx12/d3d12.h"
#include <dxgi1_4.h>
#include <d3dcompiler.h>

#include "Engine/Core/type.h"
#include "Engine/Debug/ErrorWarningAssert.hpp"
#include "Engine/File/Utils.hpp"
#include "Engine/Math/Primitives/vec3.hpp"
#include "Engine/Math/Primitives/vec4.hpp"
#include "Game/dx12/Dx12Instance.hpp"
#include "Game/dx12/Dx12Device.hpp"
#include "Game/dx12/Dx12CommandList.hpp"

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxgi.lib")
#define UNUSED(x) (void)x

// -------------------------  constant ------------------------------
constexpr uint frameCount = 2;
// ------------------------------------------------------------------

bool quit = false;
uint width = 1280;
uint height = 720;


Dx12Instance* mInstance= nullptr;
S<Dx12Device> mDevice;
S<Dx12CommandList> mContext;
void Initialize() {
  mInstance = new Dx12Instance();
  mDevice = mInstance->createDevice();
  mContext = mDevice->getImmediateCommandList();

}


void RunFrame() {
  mContext->beginFrame();

  vertex_pc_t triangleVertices1[] =
  {
    { { 0.0f, 0.25f, 0.0f },{ 1.0f, 0.0f, 0.0f, 1.0f } },
  { { 0.25f, -0.25f, 0.0f },{ 0.0f, 1.0f, 0.0f, 1.0f } },
  { { 0.f, -0.25f, 0.0f },{ 0.0f, 0.0f, 1.0f, 1.0f } }
  };

  vertex_pc_t triangleVertices2[] =
  {
    { { 0.0f, 0.25f, 0.0f },{ 1.0f, 0.0f, 1.0f, 1.0f } },
  { { 0.f, -0.25f, 0.0f },{ 1.0f, 1.0f, 0.0f, 1.0f } },
  { { -0.25f, -0.25f, 0.0f },{ 0.0f, 1.0f, 1.0f, 1.0f } }
  };
  mContext->drawVertexArray(triangleVertices1);
  mContext->drawVertexArray(triangleVertices2);

  TODO("rename to dispatchCommandList()")
  mContext->afterFrame();
}

//-----------------------------------------------------------------------------------------------
int __stdcall WinMain(HINSTANCE, HINSTANCE, LPSTR commandLineString, int) {
  UNUSED(commandLineString);
  Initialize();

  // Program main loop; keep running frames until it's time to quit
  while (!mDevice->getOutput()->quit()) {
    MSG message;
    while (PeekMessage(&message, mDevice->getOutput()->mWindow, 0, 0, PM_REMOVE)) {
      TranslateMessage(&message);
      DispatchMessage(&message);
    }
    RunFrame();
  }

  //Shutdown();
  return 0;
}


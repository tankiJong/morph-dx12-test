#define WIN32_LEAN_AND_MEAN		// Always #define this before #including <windows.h>
#include <Windows.h>

#include "d3d12.h"
#include <dxgi1_4.h>
#include <d3dcompiler.h>

#include "Engine/Core/type.h"
#include "Engine/Debug/ErrorWarningAssert.hpp"
#include "Engine/File/Utils.hpp"
#include "Engine/Math/Primitives/vec3.hpp"
#include "Engine/Math/Primitives/vec4.hpp"

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxgi.lib")
#define UNUSED(x) (void)x

#define SUCCEEDED(hr)   (((HRESULT)(hr)) >= 0)
#define FAILED(hr)      (((HRESULT)(hr)) < 0)

#define countof(arr) sizeof(arr) / sizeof(arr[0])
uint width = 1280;
uint height = 720;

// -------------------------  constant ------------------------------
constexpr uint frameCount = 2;
// ------------------------------------------------------------------

bool quit = false;

void rhiInit();

inline void breakOnFail(HRESULT hr) {
	if(FAILED(hr)) {
    __debugbreak();
	}
}

LRESULT CALLBACK windowProc(HWND window, uint message, WPARAM wparam, LPARAM lparam) {
  switch(message) {
    case WM_DESTROY:
    case WM_CLOSE:
    {
      quit = true;
      break;; // "Consumes" this message (tells Windows "okay, we handled it")
    }
    default:
      
    break;
  }
  return ::DefWindowProc(window, message, wparam, lparam);
}



void WindowsMessageHandlingProcedure(uint wmMessageCode, size_t /*wParam*/, size_t lParam) {
  UNUSED(lParam);
  switch (wmMessageCode) {
		// App close requested via "X" button, or right-click "Close Window" on task bar, or "Close" from system menu, or Alt-F4

  }
}

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




//-----------------------------------------------------------------------------------------------
HWND window;
void Initialize() {
  WNDCLASSEX windowClassDescription;
  memset(&windowClassDescription, 0, sizeof(windowClassDescription));
  windowClassDescription.cbSize = sizeof(windowClassDescription);
  windowClassDescription.style = CS_OWNDC; // Redraw on move, request own Display Context
  windowClassDescription.lpfnWndProc =windowProc;
  // Register our Windows message-handling function
  windowClassDescription.hInstance = GetModuleHandle(NULL);
  windowClassDescription.hIcon = NULL;
  windowClassDescription.hCursor = NULL;
  windowClassDescription.lpszClassName = L"dx12test";
  RegisterClassEx(&windowClassDescription);

  window = CreateWindowEx(
    WS_EX_APPWINDOW,
    L"dx12test",
    L"dx12test",
    WS_CAPTION | WS_BORDER | WS_THICKFRAME | WS_SYSMENU | WS_OVERLAPPED,
    CW_USEDEFAULT,
    CW_USEDEFAULT,
    width,
    height,
    NULL,
    NULL,
    GetModuleHandle(NULL),
    NULL);
  ShowWindow(window, SW_SHOW);
  rhiInit();
}


//-----------------------------------------------------------------------------------------------
ID3D12Debug* debugController = nullptr;

void getHardwareAdapter(IDXGIFactory2* pFactory, IDXGIAdapter1*& ppAdapter) {
  IDXGIAdapter1* adapter = nullptr;
  ppAdapter = nullptr;

  for (UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != pFactory->EnumAdapters1(adapterIndex, &adapter); ++adapterIndex) {
    DXGI_ADAPTER_DESC1 desc;
    adapter->GetDesc1(&desc);

    if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
      // Don't select the Basic Render Driver adapter.
      continue;
    }

    // Check to see if the adapter supports Direct3D 12, but don't create the
    // actual device yet.
    if (SUCCEEDED(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr))) {
      break;
    }
  }

  ppAdapter = adapter;
}

ID3D12Device* mDevice = nullptr;
ID3D12CommandQueue* mCmdQueue = nullptr;
uint mFrameIndex = 0;
IDXGISwapChain3* mSwapChain = nullptr;
ID3D12DescriptorHeap* mRtvHeap = nullptr;
ID3D12DescriptorHeap* mSrvHeap = nullptr;
uint mRtvDexcriptorSize = 0;
ID3D12Resource* mRenderTargets[frameCount];
ID3D12CommandAllocator* mCmdAllocator = nullptr;
ID3D12GraphicsCommandList* mCmdlist = nullptr;
ID3D12Fence* mFence = nullptr;
u64 mFenceVal;
HANDLE mFenceEvent;
void createPipline() {
  UINT dxgiFactoryFlags = 0;

	/****************************************
	 *
	 * enable debug layer if it's in debug mode
	 *
	 ****************************************/
#if defined(_DEBUG)
	// enable debug layer for debug mode.
	// have to do this step before create device or it will inavalidate the active device
	if(SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
    debugController->EnableDebugLayer();

    dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
	}
#endif

  /****************************************
  *
  * create render device
  *
  ****************************************/
  IDXGIFactory4* factory;
  breakOnFail(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory)));

  IDXGIAdapter1* hardwareAdapter = nullptr;
  getHardwareAdapter(factory, hardwareAdapter);

  // no hardware support for dx12 then use a wrap device
	if(hardwareAdapter == nullptr) {
    IDXGIAdapter* warpAdapter;
    breakOnFail(factory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter)));

    breakOnFail(D3D12CreateDevice(
      warpAdapter,
      D3D_FEATURE_LEVEL_11_0,
      IID_PPV_ARGS(&mDevice)
    ));
	} else {
    breakOnFail(D3D12CreateDevice(hardwareAdapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&mDevice)));
	}

  /****************************************
  *
  * create cmd queue
  * what is command queue: https://msdn.microsoft.com/en-us/library/windows/desktop/dn899124(v=vs.85).aspx#command_queue_overview
  *
  ****************************************/
  D3D12_COMMAND_QUEUE_DESC queueDesc = {};
  queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
  queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT; // https://msdn.microsoft.com/en-us/library/dn770348(v=vs.85).aspx

  breakOnFail(mDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mCmdQueue)));

  /****************************************
  *
  * create swap chain
  * What is a swap chain in DX12(not identical to earlier DX): https://msdn.microsoft.com/en-us/library/windows/desktop/dn903945(v=vs.85).aspx
  * swap chain control the buffers get swapped to the front buffer 
  * 
  ****************************************/
  DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
  swapChainDesc.BufferCount = frameCount; // front buffer & back buffer
  swapChainDesc.Width = 0;
  swapChainDesc.Height = 0; // will get figured out and fit the window, when calling `CreateSwapChainForHwnd`
  swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; // https://msdn.microsoft.com/en-us/library/hh706346(v=vs.85).aspx
  swapChainDesc.SampleDesc.Count = 1;

  IDXGISwapChain1* sc;
  breakOnFail(factory->CreateSwapChainForHwnd(mCmdQueue, window, &swapChainDesc, nullptr, nullptr, &sc));
  mSwapChain = (IDXGISwapChain3*)sc;
	// do not support fullscreen 
  breakOnFail(factory->MakeWindowAssociation(window, DXGI_MWA_NO_ALT_ENTER));

  mFrameIndex = mSwapChain->GetCurrentBackBufferIndex();

  /****************************************
  *
  * create descriptor heaps
  * https://msdn.microsoft.com/en-us/library/windows/desktop/dn899110(v=vs.85).aspx
  * A descriptor heap is a collection of contiguous allocations of descriptors, one allocation for every descriptor.
  * 
  ****************************************/

  // Describe and create a render target view (RTV) descriptor heap.
  D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
  rtvHeapDesc.NumDescriptors = frameCount;
  rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
  rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
  breakOnFail(mDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&mRtvHeap)));

  mRtvDexcriptorSize = mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

  // Describe and create a shader resource view (SRV) heap for the texture.
  D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
  srvHeapDesc.NumDescriptors = 1;
  srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
  srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
  breakOnFail(mDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&mSrvHeap)));

  // Create frame resources.
  {
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle(mRtvHeap->GetCPUDescriptorHandleForHeapStart());

    // Create a RTV for each frame.
    for (UINT n = 0; n < frameCount; n++) {
      breakOnFail(mSwapChain->GetBuffer(n, IID_PPV_ARGS(&mRenderTargets[n])));
      mDevice->CreateRenderTargetView(mRenderTargets[n], nullptr, rtvHandle); 
      rtvHandle.ptr += 1 * mRtvDexcriptorSize; // QA
    }
  }

  breakOnFail(mDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&mCmdAllocator)));
}

ID3D12RootSignature* mRootSignature = nullptr;
ID3D12PipelineState* mPipelineState = nullptr;
ID3D12Resource* mVertexBuffer = nullptr;
ID3D12Resource* mTexture = nullptr;
D3D12_VERTEX_BUFFER_VIEW mVertexBufferView;

void waitForPreviousFrame() {

  /****************************************
  *
  * wait for previous frame
  *
  ****************************************/
  // WAITING FOR THE FRAME TO COMPLETE BEFORE CONTINUING IS NOT BEST PRACTICE.
  // This is code implemented as such for simplicity. The D3D12HelloFrameBuffering
  // sample illustrates how to use fences for efficient resource usage and to
  // maximize GPU utilization.

  // Signal and increment the fence value.
  const UINT64 fence = mFenceVal;
  breakOnFail(mCmdQueue->Signal(mFence, fence));
  mFenceVal++;

  // Wait until the previous frame is finished.
  if (mFence->GetCompletedValue() < fence) {
    breakOnFail(mFence->SetEventOnCompletion(fence, mFenceEvent));
    WaitForSingleObject(mFenceEvent, INFINITE);
  }

  mFrameIndex = mSwapChain->GetCurrentBackBufferIndex();
}
void loadAsset() {
  /****************************************
  *
  * create empty root signature
  * https://msdn.microsoft.com/en-us/library/windows/desktop/mt709155(v=vs.85).aspx
  *
  ****************************************/

  // this part is in the dx example, so I will just put it here for now before I have good enough understanding to remove it.
  D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
  // This is the highest version the sample supports. If CheckFeatureSupport succeeds, the HighestVersion returned will not be greater than this.
  featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

  if (FAILED(mDevice->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData)))) {
    featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
  }

  D3D12_DESCRIPTOR_RANGE1 ranges[1];
  ranges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
  ranges[0].NumDescriptors = 1;
  ranges[0].BaseShaderRegister = 0;
  ranges[0].RegisterSpace = 0;
  ranges[0].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC;
  ranges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

  D3D12_ROOT_PARAMETER1 rootPararms[1];
  rootPararms[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
  rootPararms[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
  rootPararms[0].DescriptorTable.NumDescriptorRanges = 1;
  rootPararms[0].DescriptorTable.pDescriptorRanges = ranges;

  D3D12_STATIC_SAMPLER_DESC sampler = {};
  sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
  sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
  sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
  sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
  sampler.MipLODBias = 0;
  sampler.MaxAnisotropy = 0;
  sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
  sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
  sampler.MinLOD = 0.0f;
  sampler.MaxLOD = D3D12_FLOAT32_MAX;
  sampler.ShaderRegister = 0;
  sampler.RegisterSpace = 0;
  sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

  D3D12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
  rootSignatureDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
  rootSignatureDesc.Desc_1_1.NumParameters = 1;
  rootSignatureDesc.Desc_1_1.pParameters = rootPararms;
  rootSignatureDesc.Desc_1_1.NumStaticSamplers = 1;
  rootSignatureDesc.Desc_1_1.pStaticSamplers = &sampler;
  rootSignatureDesc.Desc_1_1.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

  ID3DBlob* signature = nullptr;
  ID3DBlob* error = nullptr;

  // possibly need to have a if statement to say whether to create v1.1 or v1.0 root signature, 
  // which is visible in helper library: D3DX12SerializeVersionedRootSignature.
  // breakOnFail(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
  breakOnFail(D3D12SerializeVersionedRootSignature(&rootSignatureDesc, &signature, &error));
  breakOnFail(mDevice->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&mRootSignature)));

  /****************************************
  *
  * create pipeline state
  *
  ****************************************/
#if defined(_DEBUG)
  // Enable better shader debugging with the graphics debugging tools.
  UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
  UINT compileFlags = 0;
#endif

  ID3DBlob* vertexShader = nullptr;
  ID3DBlob* pixelShader = nullptr;

  fs::path shaderPath = fs::absolute("shaders.hlsl");
  breakOnFail(D3DCompileFromFile(shaderPath.c_str(), nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &vertexShader, nullptr));
  breakOnFail(D3DCompileFromFile(shaderPath.c_str(), nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &pixelShader, nullptr));

  // create vertex input layout
  D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
  {
    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
  };

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

  // set up pso
  D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
  psoDesc.InputLayout = { inputElementDescs, countof(inputElementDescs) };
  psoDesc.pRootSignature = mRootSignature;
  psoDesc.VS = VS;
  psoDesc.PS = PS;
  psoDesc.RasterizerState = rasterizerDesc;
  psoDesc.BlendState = blendDesc;
  psoDesc.DepthStencilState.DepthEnable = FALSE;
  psoDesc.DepthStencilState.StencilEnable = FALSE;
  psoDesc.SampleMask = UINT_MAX;
  psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
  psoDesc.NumRenderTargets = 1;
  psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
  psoDesc.SampleDesc.Count = 1;
  breakOnFail(mDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPipelineState)));

  /****************************************
  *
  * create command list
  * https://msdn.microsoft.com/en-us/library/windows/desktop/dn899205(v=vs.85).aspx#creating_command_lists
  *
  ****************************************/
  breakOnFail(mDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, mCmdAllocator, mPipelineState, IID_PPV_ARGS(&mCmdlist)));

  /****************************************
  *
  * create vertex buffer
  *
  ****************************************/
  struct vertex_pc_t {
    vec3 position;
    vec4 color;
  };
  vertex_pc_t triangleVertices[] =
  {
    { { 0.0f, 0.25f, 0.0f },{ 1.0f, 0.0f, 0.0f, 1.0f } },
    { { 0.25f, -0.25f, 0.0f },{ 0.0f, 1.0f, 0.0f, 1.0f } },
    { { -0.25f, -0.25f, 0.0f },{ 0.0f, 0.0f, 1.0f, 1.0f } }
  };

  const UINT vertexBufferSize = sizeof(triangleVertices);

  // Note: using upload heaps to transfer static data like vert buffers is not 
  // recommended. Every time the GPU needs it, the upload heap will be marshalled 
  // over. Please read up on Default Heap usage. An upload heap is used here for 
  // code simplicity and because there are very few verts to actually transfer.
  D3D12_HEAP_PROPERTIES heapProp;
  heapProp.Type = D3D12_HEAP_TYPE_UPLOAD;
  heapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
  heapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
  heapProp.CreationNodeMask = 1;
  heapProp.VisibleNodeMask = 1;

  D3D12_RESOURCE_DESC resourceDesc;
  resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
  resourceDesc.Alignment = 0;
  resourceDesc.Width = vertexBufferSize;
  resourceDesc.Height = 1;
  resourceDesc.DepthOrArraySize = 1;
  resourceDesc.MipLevels = 1;
  resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
  resourceDesc.SampleDesc.Count = 1;
  resourceDesc.SampleDesc.Quality = 0;
  resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
  resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

  breakOnFail(mDevice->CreateCommittedResource(
    &heapProp,
    D3D12_HEAP_FLAG_NONE,
    &resourceDesc,
    D3D12_RESOURCE_STATE_GENERIC_READ,
    nullptr,
    IID_PPV_ARGS(&mVertexBuffer)));

  // Copy the triangle data to the vertex buffer.
  UINT8* pVertexDataBegin;
  D3D12_RANGE readRange;
  readRange.Begin = 0;
  readRange.End = 0;  // We do not intend to read from this resource on the CPU.
  breakOnFail(mVertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
  memcpy(pVertexDataBegin, triangleVertices, sizeof(triangleVertices));
  mVertexBuffer->Unmap(0, nullptr);

  // Initialize the vertex buffer view.
  mVertexBufferView.BufferLocation = mVertexBuffer->GetGPUVirtualAddress();
  mVertexBufferView.StrideInBytes = sizeof(vertex_pc_t);
  mVertexBufferView.SizeInBytes = vertexBufferSize;

  // Note: ComPtr's are CPU objects but this resource needs to stay in scope until
  // the command list that references it has finished executing on the GPU.
  // We will flush the GPU at the end of this method to ensure the resource is not
  // prematurely destroyed.
  ID3D12Resource* textureUploadHeap;

  D3D12_RESOURCE_DESC textureDesc = {};
  textureDesc.MipLevels = 1;
  textureDesc.MipLevels = 1;
  textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  textureDesc.Width = texWidth;
  textureDesc.Height = texHeight;
  textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
  textureDesc.DepthOrArraySize = 1;
  textureDesc.SampleDesc.Count = 1;
  textureDesc.SampleDesc.Quality = 0;
  textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

  D3D12_HEAP_PROPERTIES tempHeapProp;
  tempHeapProp.Type = D3D12_HEAP_TYPE_DEFAULT;
  tempHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
  tempHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
  tempHeapProp.CreationNodeMask = 1;
  tempHeapProp.VisibleNodeMask = 1;
  breakOnFail(mDevice->CreateCommittedResource(
    &tempHeapProp,
    D3D12_HEAP_FLAG_NONE,
    &textureDesc,
    D3D12_RESOURCE_STATE_COPY_DEST,
    nullptr,
    IID_PPV_ARGS(&mTexture)));

  // get required size of buffer
  // helper function in library: GetRequiredIntermediateSize
  u64 uploadBufferSize;
  {
    D3D12_RESOURCE_DESC Desc = mTexture->GetDesc();
    UINT64 RequiredSize = 0;

    ID3D12Device* pDevice;
    mTexture->GetDevice(__uuidof(*pDevice), reinterpret_cast<void**>(&pDevice));
    pDevice->GetCopyableFootprints(&Desc, 0, 1, 0, nullptr, nullptr, nullptr, &RequiredSize);
    pDevice->Release();

    uploadBufferSize = RequiredSize;
  }

  D3D12_RESOURCE_DESC textureResourceDesc;
  textureResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
  textureResourceDesc.Alignment = 0;
  textureResourceDesc.Width = uploadBufferSize;
  textureResourceDesc.Height = 1;
  textureResourceDesc.DepthOrArraySize = 1;
  textureResourceDesc.MipLevels = 1;
  textureResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
  textureResourceDesc.SampleDesc.Count = 1;
  textureResourceDesc.SampleDesc.Quality = 0;
  textureResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
  textureResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

  breakOnFail(mDevice->CreateCommittedResource(&heapProp, D3D12_HEAP_FLAG_NONE, 
              &textureResourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&textureUploadHeap)));

  // Copy data to the intermediate upload heap and then schedule a copy 
  // from the upload heap to the Texture2D.
  std::vector<UINT8> texture = GenerateTextureData();
  D3D12_SUBRESOURCE_DATA textureData = {};
  textureData.pData = &texture[0];
  textureData.RowPitch = texWidth * texHeight;
  textureData.SlicePitch = textureData.RowPitch * texHeight;

  UpdateSubresources(mCmdlist, mTexture, textureUploadHeap, 0, 0, 1, &textureData);
  mCmdlist->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mTexture, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

  // Describe and create a SRV for the texture.
  D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
  srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  srvDesc.Format = textureDesc.Format;
  srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
  srvDesc.Texture2D.MipLevels = 1;
  mDevice->CreateShaderResourceView(mTexture, &srvDesc, mSrvHeap->GetCPUDescriptorHandleForHeapStart());

  // Close the command list and execute it to begin the initial GPU setup.
  breakOnFail(mCmdlist->Close());
  ID3D12CommandList* ppCommandLists[] = { mCmdlist };
  mCmdQueue->ExecuteCommandLists(countof(ppCommandLists), ppCommandLists);
  /****************************************
  *
  * create synchronization objects
  *
  ****************************************/
  breakOnFail(mDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence)));
  mFenceVal = 1;
  mFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
  if(mFenceEvent == nullptr) {
    breakOnFail(HRESULT_FROM_WIN32(GetLastError()));
  }

  // Wait for the command list to execute; we are reusing the same command 
  // list in our main loop but for now, we just want to wait for setup to 
  // complete before continuing.
  waitForPreviousFrame();
}

void rhiInit() {
  createPipline();
  loadAsset();
}
//-----------------------------------------------------------------------------------------------
void Shutdown() {
	// Destroy the global App instance
	// delete window;
	// window = nullptr;
}

D3D12_VIEWPORT mViewPoint;
D3D12_RECT mScissorRect;
//-----------------------------------------------------------------------------------------------
void RunFrame() {
  mViewPoint.TopLeftX = 0;
  mViewPoint.TopLeftY = 0;
  mViewPoint.Width = width;
  mViewPoint.Height = height;
  mViewPoint.MinDepth = D3D12_MIN_DEPTH;
  mViewPoint.MaxDepth = D3D12_MAX_DEPTH;

  mScissorRect.left = 0;
  mScissorRect.top = 0;
  mScissorRect.right = width;
  mScissorRect.bottom = height;
  /****************************************
  *
  * populate command list
  *
  ****************************************/

	// Command list allocators can only be reset when the associated 
	// command lists have finished execution on the GPU; apps should use 
	// fences to determine GPU execution progress.
  breakOnFail(mCmdAllocator->Reset());

	// However, when ExecuteCommandList() is called on a particular command 
	// list, that command list can then be reset at any time and must be before 
	// re-recording.
  breakOnFail(mCmdlist->Reset(mCmdAllocator, mPipelineState));

  // set necessary state
  mCmdlist->SetGraphicsRootSignature(mRootSignature);

  ID3D12DescriptorHeap* ppHeaps[] = { mSrvHeap };
  mCmdlist->SetDescriptorHeaps(countof(ppHeaps), ppHeaps);
  mCmdlist->SetGraphicsRootDescriptorTable(0, mSrvHeap->GetGPUDescriptorHandleForHeapStart());

  mCmdlist->RSSetViewports(1, &mViewPoint);
  mCmdlist->RSSetScissorRects(1, &mScissorRect);

  
  // Indicate that the back buffer will be used as a render target.
  D3D12_RESOURCE_BARRIER result;
  ZeroMemory(&result, sizeof(result));
  D3D12_RESOURCE_BARRIER &barrier = result;
  result.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  result.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE; // default to this
  barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES; // default to this
  barrier.Transition.pResource = mRenderTargets[mFrameIndex];
  barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
  barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;

	// Indicate that the back buffer will be used as a render target.
	mCmdlist->ResourceBarrier(1, &result);

	// QA
  D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle(mRtvHeap->GetCPUDescriptorHandleForHeapStart());
	rtvHandle.ptr += mFrameIndex * mRtvDexcriptorSize;

  mCmdlist->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

	// Record commands.
	const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
	mCmdlist->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

  mCmdlist->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  mCmdlist->IASetVertexBuffers(0, 1, &mVertexBufferView);
  mCmdlist->DrawInstanced(3, 1, 0, 0);

	// Indicate that the back buffer will now be used to present.
  result.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
  result.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	mCmdlist->ResourceBarrier(1, &result);

  breakOnFail(mCmdlist->Close());


  /****************************************
  *
  * execute command list
  *
  ****************************************/
  ID3D12CommandList* ppCommandLists[] = { mCmdlist };
  mCmdQueue->ExecuteCommandLists(countof(ppCommandLists), ppCommandLists);

  breakOnFail(mSwapChain->Present(1, 0));

  waitForPreviousFrame();
}

//-----------------------------------------------------------------------------------------------
int __stdcall WinMain(HINSTANCE, HINSTANCE, LPSTR commandLineString, int) {
	UNUSED(commandLineString);
	Initialize();

	// Program main loop; keep running frames until it's time to quit
	while (!quit)
	{
	  MSG message;
		while(PeekMessage(&message, window, 0,0, PM_REMOVE)) {
      TranslateMessage(&message);
      DispatchMessage(&message);
		}
	  RunFrame();
	}

	Shutdown();
	return 0;
}



#pragma once
#include "d3d12.h"
#include "Engine/Renderer/type.h"
#include <string>
#include <comdef.h>
#include <d3dcompiler.h>
#include <dxgi1_4.h>
#include <dxgiformat.h>

#define HR_SUCCEEDED(hr)   (((HRESULT)(hr)) >= 0)
#define HR_FAILED(hr)      (((HRESULT)(hr)) < 0)
#define EXPECT_HR_SUCCESSED(hr) EXPECTS(HR_SUCCEEDED(hr))

#define MAKE_SMART_COM_PTR(_a) _COM_SMARTPTR_TYPEDEF(_a, __uuidof(_a))

MAKE_SMART_COM_PTR(ID3D12Device);
MAKE_SMART_COM_PTR(ID3D12Debug);
MAKE_SMART_COM_PTR(ID3D12CommandQueue);
MAKE_SMART_COM_PTR(ID3D12CommandAllocator);
MAKE_SMART_COM_PTR(ID3D12GraphicsCommandList);
MAKE_SMART_COM_PTR(ID3D12DescriptorHeap);
MAKE_SMART_COM_PTR(ID3D12Resource);
MAKE_SMART_COM_PTR(ID3D12Fence);
MAKE_SMART_COM_PTR(ID3D12PipelineState);
MAKE_SMART_COM_PTR(ID3D12ShaderReflection);
MAKE_SMART_COM_PTR(ID3D12RootSignature);
MAKE_SMART_COM_PTR(ID3D12QueryHeap);
MAKE_SMART_COM_PTR(ID3D12CommandSignature);
MAKE_SMART_COM_PTR(IUnknown);

void breakOnFail(HRESULT hr);

std::wstring make_wstring(const std::string& str);

DXGI_FORMAT toDXGIFormat(eDataDeclType declType, uint count, bool normalized);

/*****************RHI typedef********************************/
using device_handle_t = ID3D12DevicePtr;
using rhi_resource_handle_t = ID3D12ResourcePtr;

using fence_handle_t = ID3D12FencePtr;
/*************************************************/
#pragma once
#include "d3d12.h"
#include <string>
#include "Engine/Renderer/type.h"
#define HR_SUCCEEDED(hr)   (((HRESULT)(hr)) >= 0)
#define HR_FAILED(hr)      (((HRESULT)(hr)) < 0)
#define EXPECT_HR_SUCCESSED(hr) EXPECTS(HR_SUCCEEDED(hr))

void breakOnFail(HRESULT hr);

std::wstring make_wstring(const std::string& str);

DXGI_FORMAT toDXGIFormat(eDataDeclType declType, uint count, bool normalized);
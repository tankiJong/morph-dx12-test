#pragma once
#include "d3d12.h"
#include <string>
#define HR_SUCCEEDED(hr)   (((HRESULT)(hr)) >= 0)
#define HR_FAILED(hr)      (((HRESULT)(hr)) < 0)
#define EXPECT_HR_SUCCESSED(hr) EXPECTS(HR_SUCCEEDED(hr))
inline void breakOnFail(HRESULT hr) {
  if (FAILED(hr)) {
    __debugbreak();
  }
}


std::wstring make_wstring(const std::string& str);
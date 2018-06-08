#include "dx12util.hpp"
#include <string>
#include <locale.h> 
std::wstring make_wstring(const std::string& str) {
  setlocale(LC_CTYPE, "");
  wchar_t p[1024];
  size_t len;
  mbstowcs_s(&len, p, str.data(), 1024);
  std::wstring str1(p);
  return str1;
}
//
// Created by TD on 2021/5/9.
//
#pragma once
#include <DoodleLib_fwd.h>

namespace doodle{
template<>
std::string ConvStr(const wxString& str){
  return str.ToStdString(wxConvUTF8);
}

template<>
wxString ConvStr(const std::string& str){
  return wxString::FromUTF8(str);
}

template<>
wxString ConvStr(const char *(str)){
  return wxString::FromUTF8(str);
}
}

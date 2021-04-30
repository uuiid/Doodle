//
// Created by TD on 2021/4/30.
//
#include <doodle_GUI/doodle_global.h>

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

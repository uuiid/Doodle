/*
 * @Author: your name
 * @Date: 2020-09-28 14:13:33
 * @LastEditTime: 2020-12-01 14:36:15
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\doodle_GUI\doodle_global.h
 */
#pragma once

#include <corelib/core_global.h>
#include <doodle_GUI/core_cpp_qt.h>

DOODLE_NAMESPACE_S
class Doodle;
enum class filterState {
  useFilter,
  notFilter,
  showAll,
};

template <typename SSC,typename SSN>
SSC ConvStr(const SSN& str){
    return  SSC{str};
}

template <>
std::string ConvStr(const wxString& str){
    return str.ToStdString(wxConvUTF8);
}

template <>
wxString ConvStr(const std::string& str){
    return wxString::FromUTF8(str);
}


DOODLE_NAMESPACE_E

wxDECLARE_APP(doodle::Doodle);
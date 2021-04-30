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

template<>
std::string ConvStr(const wxString& str);

template<>
wxString ConvStr(const std::string& str);
//模板特化一个指针类型的模板
template <typename SSC,typename SSN>
SSC ConvStr(const SSN* str){
  return  SSC{str};
}
//继续特化一个char*的平常用的
template<>
wxString ConvStr(const char *(str));

//template <typename SSC,typename SSN,std::size_t N>
//SSC ConvStr(const SSN (&str)[N]){
//  return  SSC{str};
//}
//
//继续特化一个char*的平常用的
template<std::size_t N>
wxString ConvStr(const char (&str)[N]){
    return wxString::FromUTF8(str,N);
};
DOODLE_NAMESPACE_E

wxDECLARE_APP(doodle::Doodle);
//
// Created by TD on 2021/6/17.
//

#include "static_value.h"
namespace doodle{
std::string staticValue::fun_obj() {
  static std::string install{"fun"};
  return install;
}
std::string staticValue::server_obj() {
  static std::string install{"server"};
  return install;
}
std::string staticValue::ue_path_obj() {
  static std::string install{"Engine/Binaries/Win64/UE4Editor.exe"};
  return install;
}
}

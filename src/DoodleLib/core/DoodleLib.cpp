//
// Created by TD on 2021/6/17.
//

#include "DoodleLib.h"
namespace doodle{

DoodleLib* DoodleLib::p_install = nullptr;

DoodleLib::DoodleLib() {
}
DoodleLib& DoodleLib::Get() {
  return *p_install;
}
ThreadPoolPtr DoodleLib::get_thread_pool() {
  return doodle::ThreadPoolPtr();
}
DoodleLibPtr make_doodle_lib() {
  auto ptr = std::unique_ptr<DoodleLib>(new DoodleLib{});
  DoodleLib::p_install = ptr.get();
  return ptr;
}
}

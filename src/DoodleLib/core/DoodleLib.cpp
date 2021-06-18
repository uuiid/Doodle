//
// Created by TD on 2021/6/17.
//

#include "DoodleLib.h"

#include <threadPool/ThreadPool.h>
namespace doodle {

DoodleLib* DoodleLib::p_install = nullptr;

DoodleLib::DoodleLib()
    : p_thread_pool(std::make_shared<ThreadPool>(4)) {
}
DoodleLib& DoodleLib::Get() {
  return *p_install;
}
ThreadPoolPtr DoodleLib::get_thread_pool() {
  return p_thread_pool;
}
[[maybe_unused]] DoodleLibPtr make_doodle_lib() {
  auto ptr             = std::unique_ptr<DoodleLib>(new DoodleLib{});
  DoodleLib::p_install = ptr.get();
  return ptr;
}
}  // namespace doodle

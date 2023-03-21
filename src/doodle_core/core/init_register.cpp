#include "init_register.h"

#include <doodle_core/core/core_sig.h>
#include <doodle_core/core/doodle_lib.h>
#include <doodle_core/logger/logger.h>

#include <boost/asio.hpp>
namespace doodle {

std::vector<std::function<void()>>& init_register::registered_functions() { return init_p; }
void init_register::reg_class() {
  DOODLE_LOG_INFO("开始反射注册");
  auto& l_map = registered_functions();
  for (auto&& l_fun : l_map) {
    l_fun();
  }
  DOODLE_LOG_INFO("结束开始反射注册");
  for (auto&& mat : entt::resolve()) DOODLE_LOG_INFO(fmt::format("{}", mat.second.info().name()));
}
init_register& init_register::instance() noexcept {
  static init_register l_r{};
  return l_r;
}
init_register::init_register()  = default;
init_register::~init_register() = default;

}  // namespace doodle

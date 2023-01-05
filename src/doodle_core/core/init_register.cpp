#include "init_register.h"

#include <doodle_core/core/core_sig.h>
#include <doodle_core/core/doodle_lib.h>
#include <doodle_core/logger/logger.h>

#include <boost/asio.hpp>
namespace doodle {

std::multimap<std::int32_t, std::function<void()>>& init_register::registered_functions() { return init_p; }
void init_register::reg_class() {
  auto l_s = boost::asio::make_strand(g_io_context());
  DOODLE_LOG_INFO("开始反射注册");

  auto& l_map = registered_functions();
  for (auto it = l_map.begin(), end = l_map.end(); it != end; it = l_map.upper_bound(it->first)) {
    DOODLE_LOG_INFO("初始化优先级 {}", it->first);
    auto l_p = init_register::instance().registered_functions().equal_range(it->first);
    std::for_each(l_p.first, l_p.second, [](const std::multimap<std::int32_t, std::function<void()>>::value_type& i) {
      i.second();
    });
  }
  DOODLE_LOG_INFO("结束开始反射注册");
  boost::asio::post(l_s, [l_s]() {
    for (auto&& mat : entt::resolve()) DOODLE_LOG_INFO(fmt::format("{}", mat.info().name()));
  });
}
init_register& init_register::instance() noexcept {
  static init_register l_r{};
  return l_r;
}
void init_register::init_run() {
  for (auto&& ref_ : get_derived_class<base_registrar>()) {
    auto l_i = ref_.construct();
    l_i.cast<base_registrar&>().init();
  }
}
init_register::init_register()  = default;
init_register::~init_register() = default;

}  // namespace doodle

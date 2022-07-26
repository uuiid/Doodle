#include "init_register.h"
#include <doodle_core/core/core_sig.h>
#include <doodle_core/core/doodle_lib.h>
#include <doodle_core/thread_pool/process_pool.h>
#include <doodle_core/thread_pool/asio_pool.h>
#include <doodle_core/logger/logger.h>
#include <doodle_core/gui_template/gui_process.h>

namespace doodle {

std::multimap<std::int32_t, std::function<void()>>& init_register::registered_functions() {
  return init_p;
}
void init_register::reg_class() {
  auto l_next = make_process_adapter(g_io_context().get_executor(),
                                     []() { DOODLE_LOG_INFO("开始反射注册"); });

  auto& l_map = registered_functions();
  for (auto it = l_map.begin(), end = l_map.end();
       it != end;
       it = l_map.upper_bound(it->first)) {
    DOODLE_LOG_INFO("初始化优先级 {}", it->first);
    l_next.next([key = it->first, this]() {
      auto l_p = registered_functions().equal_range(key);
      std::for_each(l_p.first, l_p.second,
                    [](const std::multimap<std::int32_t, std::function<void()>>::value_type& i) {
                      i.second();
                    });
    });
  }
  l_next.next([&]() {
    DOODLE_LOG_INFO("结束开始反射注册");
    for (auto&& mat : entt::resolve())
      DOODLE_LOG_INFO(fmt::format("{}", mat.info().name()));
    g_reg()->ctx().at<core_sig>().init_end();
  });

  boost::asio::post(l_next);
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

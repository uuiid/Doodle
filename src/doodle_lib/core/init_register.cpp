#include "init_register.h"
#include <core/core_sig.h>
#include <long_task/process_pool.h>
namespace doodle {

std::multimap<std::int32_t, std::function<void()>>& init_register::registered_functions() {
  static std::multimap<std::int32_t, std::function<void()>> rel{};
  return rel;
}
void init_register::begin_init() {
  auto l_then = g_main_loop().attach<one_process_t>([]() {
    DOODLE_LOG_INFO("开始初始化队列");
  });
  for (auto&& i : registered_functions()) {
    l_then = l_then.then<one_process_t>([&]() {
      DOODLE_LOG_INFO("初始化优先级 {}", i.first);
      i.second();
    });
  }
  l_then.then<one_process_t>([&]() {
    DOODLE_LOG_INFO("结束初始化");
    g_reg()->ctx<core_sig>().init_end();
  });
}
init_register::init_register()  = default;
init_register::~init_register() = default;

}  // namespace doodle

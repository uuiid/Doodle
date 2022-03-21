#include "init_register.h"

namespace doodle {

std::multimap<std::int32_t, std::function<void()>>& init_register::registered_functions() {
  static std::multimap<std::int32_t, std::function<void()>> rel{};
  return rel;
}
void init_register::begin_init() {
  for (auto&& i : registered_functions()) {
    i.second();
  }
}
init_register::init_register()  = default;
init_register::~init_register() = default;

}  // namespace doodle

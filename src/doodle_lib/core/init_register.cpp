#include "init_register.h"

namespace doodle {

std::multimap<std::int32_t, std::function<void()>>& init_register::egistered_functions() {
  static std::multimap<std::int32_t, std::function<void()>> rel{};
  return rel;
}

}  // namespace doodle
//
// Created by TD on 2022/1/21.
//

#pragma once
#include <doodle_lib/doodle_lib_fwd.h>

namespace doodle {

class init_register {
 public:
  static std::multimap<std::int32_t,
                       std::function<void()>>&
  registered_functions();

  static void begin_init();

  template <class T>
  struct registrar {
    friend T;

    static bool register_() {
      const auto l_priority = T::priority;
      registered_functions().insert(l_priority, T{});
      return true;
    }
    [[maybe_unused]] static bool registered;

   private:
    registrar() { (void)registered; }
  };

 private:
 public:
  init_register();
  virtual ~init_register();
};
template <typename T>
bool init_register::registrar<T>::registered =
    init_register::registrar<T>::register_();
}  // namespace doodle

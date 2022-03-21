//
// Created by TD on 2022/1/21.
//

#pragma once
#include <doodle_lib/doodle_lib_fwd.h>

namespace doodle {

class init_register {
 public:
 private:
  std::multimap<std::int32_t,
                std::function<void()>>&
  egistered_functions();

  template <std::int32_t PRIORITY, typename FUN>
  class init_fun {
    constexpr static const std::int32_t p_pri{PRIORITY};

   public:
    init_fun() {
      (void)registered;
    };
    static bool register_fun() {
      egistered_functions().insert(p_pri, FUN);
      return true;
    };
    static bool registered;
  };

 public:
  init_register();
  virtual ~init_register();
};
template <typename FUN>
bool init_register::init_fun<FUN>::registered =
    init_register::init_fun<FUN>::register_fun();

}  // namespace doodle
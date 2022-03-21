//
// Created by TD on 2022/1/21.
//

#pragma once
#include <doodle_lib/doodle_lib_fwd.h>

namespace doodle {

class init_register {
  std::multimap<std::int32_t,
                std::function<void()>>
      init_p;

 public:
  std::multimap<std::int32_t,
                std::function<void()>>&
  registered_functions();

  void begin_init();

  template <class T>
  struct registrar {
   private:
    static T& register_() {
      const auto l_priority = T::priority;
      static T l_t{};
      (void)registered;
      instance().registered_functions().insert(
          std::make_pair(l_priority, [&]() { l_t(); }));
      return l_t;
    }

   public:
    friend T;
    static T& getInstance() {
      return register_();
    }
    inline static T& registered = getInstance();

    registrar() { (void)registered; }

   private:
  };

 private:
  init_register();

 public:
  virtual ~init_register();
  static init_register& instance() noexcept;
};
// template <typename T>
// T& init_register::registrar<T>::registered =
//     init_register::registrar<T>::getInstance();
}  // namespace doodle

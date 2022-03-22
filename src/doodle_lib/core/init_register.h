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

  using reg_fun = void (*)();
  template <reg_fun Fun, std::int32_t priority_t>
  struct registrar_lambda {
    constexpr static const std::int32_t priority{priority_t};

   private:
    static registrar_lambda& register_() {
      static registrar_lambda l_t{};
      (void)registered;
      instance().registered_functions().insert(
          std::make_pair(priority, [&]() { Fun(); }));
      return l_t;
    }

   public:
    static registrar_lambda& getInstance() {
      return register_();
    }
    inline static registrar_lambda& registered = getInstance();

    registrar_lambda() { (void)registered; }
  };

  class base_registrar {
   public:
    virtual ~base_registrar(){};
    virtual void init() const = 0;
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

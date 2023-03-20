//
// Created by TD on 2022/1/21.
//

#pragma once
#include <doodle_core/doodle_core_fwd.h>

#include <map>

namespace doodle {

class DOODLE_CORE_API init_register {
  std::vector<std::function<void()>> init_p;

 public:
  std::vector<std::function<void()>>& registered_functions();

  void reg_class();

 private:
  init_register();

 public:
  virtual ~init_register();
  static init_register& instance() noexcept;
};
}  // namespace doodle

#define DOODLE_REGISTER_BEGIN()                                                                                       \
  void BOOST_PP_CAT(doodle_reg, __LINE__)();                                                                          \
  namespace {                                                                                                         \
  struct registrar_lambda {                                                                                           \
   public:                                                                                                            \
    static bool getInstance() {                                                                                       \
      registered;                                                                                                     \
      init_register::instance().registered_functions().emplace_back([&]() { BOOST_PP_CAT(doodle_reg, __LINE__)(); }); \
      return true;                                                                                                    \
    }                                                                                                                 \
    static bool registered;                                                                                           \
                                                                                                                      \
    registrar_lambda() { registered; }                                                                                \
  };                                                                                                                  \
  bool registrar_lambda::registered{registrar_lambda::getInstance()};                                                 \
  }                                                                                                                   \
  void BOOST_PP_CAT(doodle_reg, __LINE__)()

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
namespace details {
template <typename T>
struct registrar_lambda {};
}  // namespace details

}  // namespace doodle

#define DOODLE_REGISTER_BEGIN(class_name)                                                     \
  template <>                                                                                 \
  struct ::doodle::details::registrar_lambda<class_name> {                                    \
    static void doodle_reg();                                                                 \
                                                                                              \
   public:                                                                                    \
    static bool getInstance() {                                                               \
      registered;                                                                             \
      init_register::instance().registered_functions().emplace_back([&]() { doodle_reg(); }); \
      return true;                                                                            \
    }                                                                                         \
    static bool registered;                                                                   \
                                                                                              \
    registrar_lambda() { registered; }                                                        \
  };                                                                                          \
  bool ::doodle::details::registrar_lambda<class_name>::registered{                           \
      ::doodle::details::registrar_lambda<class_name>::getInstance()};                        \
  void ::doodle::details::registrar_lambda<class_name>::doodle_reg()

//
// Created by TD on 2022/1/21.
//

#pragma once
#include <doodle_core/doodle_core_fwd.h>

#include <map>

namespace doodle {

class DOODLE_CORE_API init_register {
  std::multimap<std::int32_t, std::function<void()>>
      init_p;

 public:
  std::multimap<std::int32_t, std::function<void()>>&
  registered_functions();

  void reg_class();
  void init_run();

  using reg_fun = void (*)();
  template <reg_fun Fun, std::int32_t priority_t>
  struct registrar_lambda {
    constexpr static const std::int32_t priority{priority_t};

   public:
    static bool getInstance() {
      registered;
      instance().registered_functions().insert(
          std::make_pair(priority, [&]() { Fun(); })
      );
      return true;
    }
    inline static bool registered = getInstance();

    registrar_lambda() {
      registered;
    }
  };

  class DOODLE_CORE_API base_registrar {
   public:
    virtual ~base_registrar() = default;
    virtual void init() const = 0;
  };

 private:
  init_register();

 public:
  virtual ~init_register();
  static init_register& instance() noexcept;

  template <typename Base_T>
  std::vector<entt::meta_type> get_derived_class() {
    std::vector<entt::meta_type> derived_list{};
    for (auto&& ref_ : entt::resolve()) {
      for (auto&& l_base : ref_.base()) {
        if (l_base == entt::resolve<Base_T>()) {
          derived_list.push_back(ref_);
          break;
        }
      }
    }
    return derived_list;
  }
};

namespace init_register_ns {
constexpr auto meta_init_registrar_lab = []() {
  entt::meta<init_register::base_registrar>()
      .type()
      .func<&init_register::base_registrar::init>("init"_hs);
};
class meta_init_registrar
    : public init_register::registrar_lambda<meta_init_registrar_lab, 1> {};
}  // namespace init_register_ns
}  // namespace doodle

#define DOODLE_REGISTER_BEGEN(class_name) \
  namespace class_name##_ns {             \
    constexpr auto meta_init_registrar_lab = []() {                              \
      entt::meta<class_name::class_name>()                                       \
          .type()
#define DOODLE_REGISTER_END(index)                                                 \
  }                                                                                \
  ;                                                                                \
  class meta_init_registrar                                                        \
      : public init_register::registrar_lambda<meta_init_registrar_lab, index> {}; \
  }

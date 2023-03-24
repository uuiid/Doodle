//
// Created by TD on 2022/9/30.
//

#pragma once

#include <doodle_core/doodle_core_fwd.h>

namespace doodle::details {

class DOODLE_CORE_API app_facet_interface_1 : public entt::type_list<bool(), void()> {
 public:
  template <typename Base>
  struct type : Base {
    bool post() { entt::poly_call<0>(*this); }
    void add_program_options() { entt::poly_call<1>(*this); }
  };

  template <typename Type>
  using impl = entt::value_list<&Type::post, &Type::add_program_options>;
};

}  // namespace doodle::details

namespace doodle {
using app_facet_interface = entt::poly<details::app_facet_interface_1>;
}

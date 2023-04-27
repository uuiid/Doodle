//
// Created by td_main on 2023/4/27.
//

#pragma once

#include <entt/entt.hpp>

namespace doodle::maya_plug {
namespace details {

class cloth_interface
    : public entt::type_list<void(const FSys::path& path) const, void() const, void(const entt::handle&) const> {
 public:
  template <typename base_t>
  struct type : base_t {
    void set_cache_folder(const FSys::path& in_path) const { return entt::poly_call<0>(*this, in_path); };
    void sim_cloth() const { return entt::poly_call<1>(*this); };
    void add_field(const entt::handle& in_handle) const { return entt::poly_call<2>(*this, in_handle); };
  };

  template <typename type_t>
  using impl = entt::value_list<&type_t::set_cache_folder, &type_t::sim_cloth, &type_t::add_field>;
};

}  // namespace details

using cloth_interface = entt::poly<details::cloth_interface>;

}  // namespace doodle::maya_plug
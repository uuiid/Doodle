//
// Created by td_main on 2023/4/27.
//

#pragma once

#include <doodle_core/core/file_sys.h>

#include "entt/entity/fwd.hpp"
#include "maya/MApiNamespace.h"
#include <entt/entt.hpp>
#include <memory>
#include <vector>

namespace doodle::maya_plug {
class reference_file;
namespace details {

class cloth_interface {
 public:
  virtual void set_cache_folder(const entt::handle& in_handle, const FSys::path& in_path) const = 0;
  virtual MObject get_solver(const entt::handle_view<reference_file>& in_handle) const          = 0;
  virtual void sim_cloth() const                                                                = 0;
  virtual void add_field(const entt::handle& in_handle) const                                   = 0;
  virtual void add_collision(const entt::handle& in_handle) const                               = 0;
  virtual void rest(const entt::handle& in_handle) const                                        = 0;
  virtual void clear_cache() const                                                              = 0;
  inline void set_cache_folder(const entt::handle& in_handle) const { set_cache_folder(in_handle, FSys::path{}); }
};

class cloth_factory_interface {
 public:
  virtual std::vector<entt::handle> create_cloth() const = 0;
};

}  // namespace details

using cloth_interface         = std::shared_ptr<details::cloth_interface>;
using cloth_factory_interface = std::shared_ptr<details::cloth_factory_interface>;

}  // namespace doodle::maya_plug
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
  /**
   * @brief 设置qcloth缓存路径,如果存在缓存文件,还会删除缓存文件
   */
  virtual void set_cache_folder(const entt::handle& in_handle, const FSys::path& in_path, bool need_clear) const = 0;
  [[nodiscard]] virtual MObject get_solver() const                                                               = 0;
  virtual void sim_cloth() const                                                                                 = 0;
  virtual void add_field(const entt::handle& in_handle) const                                                    = 0;
  virtual void add_collision(const entt::handle& in_handle) const                                                = 0;
  virtual void rest(const entt::handle& in_handle) const                                                         = 0;
  [[nodiscard]] virtual std::string get_namespace() const                                                        = 0;
  virtual void cover_cloth_attr(const entt::handle& in_handle) const                                             = 0;
  [[nodiscard]] virtual MDagPath get_shape() const                                                               = 0;
  inline void set_cache_folder(const entt::handle& in_handle, bool need_clear) const {
    set_cache_folder(in_handle, FSys::path{}, need_clear);
  }
  // 设置缓存为只读
  virtual void set_cache_folder_read_only(const entt::handle& in_handle) const = 0;
};

class cloth_factory_interface {
 public:
  [[nodiscard]] virtual std::vector<entt::handle> create_cloth() const = 0;
};

}  // namespace details

using cloth_interface         = std::shared_ptr<details::cloth_interface>;
using cloth_factory_interface = std::shared_ptr<details::cloth_factory_interface>;

}  // namespace doodle::maya_plug
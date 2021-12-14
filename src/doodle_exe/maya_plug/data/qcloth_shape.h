//
// Created by TD on 2021/12/6.
//

#pragma once
#include <doodle_lib/doodle_lib_fwd.h>
#include <maya/MObject.h>
namespace doodle::maya_plug {
class reference_file;
namespace qcloth_shape_n {
class low_shape;
class high_shape;
}  // namespace qcloth_shape_n

namespace qcloth_shape_n {
class low_shape {
 public:
  MObject obj;
  std::vector<entt::handle> high_shape_list;
};

class high_shape {
 public:
  MObject obj;
};
}  // namespace qcloth_shape_n

class qcloth_shape {
 private:
  entt::handle p_ref_file;
  MObject obj;

 public:
  qcloth_shape();
  explicit qcloth_shape(const entt::handle& in_ref_file, const MObject& in_object);

  /**
   * @brief 设置qcloth缓存路径,如果存在缓存文件,还会删除缓存文件
   * @return 完成设置
   */
  bool set_cache_folder() const;
  bool create_cache() const;

  void create_sim_cloth(const entt::handle& in_low_spahe);
};
}  // namespace doodle::maya_plug

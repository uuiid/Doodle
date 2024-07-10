//
// Created by TD on 2023/12/20.
//

#pragma once
#include <doodle_core/core/file_sys.h>
#include <doodle_core/doodle_core.h>
#include <doodle_core/metadata/assets.h>
#include <doodle_core/metadata/project.h>

#include <entt/entt.hpp>
#include <map>

namespace doodle::details {
class scan_category_data_t;
enum class assets_type_enum {
  scene,
  prop,
  character,
  rig,
  animation,
  fvx,
  fcx,
  other,
};
class scan_category_data_t {
 private:
  entt::handle get_project_handle() const;

  entt::handle get_assets_handle() const;

 public:

  using assets_type_enum = assets_type_enum;

  virtual ~scan_category_data_t() = default;
  // uuid和文件路径的类
  struct uuid_path_t {
    uuid uuid_;
    FSys::path path_;
    // 文件修改时间
    FSys::file_time_type last_write_time_;
  };
  using project_root_t = project;

  // ue文件
  uuid_path_t ue_file_;
  // rig文件
  uuid_path_t rig_file_;
  // 解算文件
  uuid_path_t solve_file_;

  // 这个是查找依据的根本路径, 必然不空, 可被打开
  FSys::path base_path_;

  // 项目根目录
  project_root_t project_root_;
  // 季数
  season season_;
  // 名称
  std::string name_;
  // 版本名称
  std::string version_name_;
  // 所属类别
  assets file_type_;
  // 类型
  assets_type_enum assets_type_;
  // 根据属性创建句柄, 返回值中只有新创建的句柄, 没有在数据表内的旧的
  std::vector<entt::handle> create_handles(
      const std::map<uuid, entt::handle>& in_handle_map, entt::registry& in_reg = *g_reg()
  ) const;
};
using scan_category_data_ptr = std::shared_ptr<scan_category_data_t>;
class scan_category_t {
 public:
  using project_root_t = scan_category_data_t::project_root_t;

  logger_ptr logger_;
  scan_category_t() {}
  virtual ~scan_category_t()                                                            = default;
  virtual std::vector<scan_category_data_ptr> scan(const project_root_t& in_root) const = 0;
};

NLOHMANN_JSON_SERIALIZE_ENUM(
    assets_type_enum,
    {
        {assets_type_enum::scene, "scene"},
        {assets_type_enum::prop, "prop"},
        {assets_type_enum::character, "character"},
        {assets_type_enum::rig, "rig"},
        {assets_type_enum::animation, "animation"},
        {assets_type_enum::fvx, "fvx"},
        {assets_type_enum::fcx, "fcx"},
        {assets_type_enum::other, "other"},
    }
);

}  // namespace doodle::details
//
// Created by TD on 2023/12/20.
//

#pragma once
#include <doodle_core/core/file_sys.h>
#include <doodle_core/doodle_core.h>
#include <doodle_core/metadata/assets.h>

#include <entt/entt.hpp>
#include <map>

namespace doodle::gui::details {
class scan_category_data_t;

class scan_category_data_t {
 public:
  // uuid和文件路径的类
  struct uuid_path_t {
    uuid uuid_;
    FSys::path path_;
    // 文件修改时间
    std::time_t last_write_time_;
  };
  struct project_root_t {
    FSys::path path_;
    std::string name_;
  };

  // ue文件
  uuid_path_t ue_file_;
  // maya文件
  uuid_path_t maya_file_;
  // rig文件
  uuid_path_t rig_file_;
  // 解算文件
  uuid_path_t solve_file_;

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
};
using scan_category_data_ptr = std::shared_ptr<scan_category_data_t>;
class scan_category_t {
 public:
  using project_root_t = scan_category_data_t::project_root_t;

  logger_ptr logger_;
  scan_category_t() {}
  virtual ~scan_category_t()                                                            = default;
  virtual std::vector<scan_category_data_ptr> scan(const project_root_t& in_root) const = 0;
  virtual std::vector<scan_category_data_ptr> check_path(const project_root_t& in_root, entt::handle& in_path)
      const = 0;
};

}  // namespace doodle::gui::details
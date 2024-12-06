//
// Created by TD on 2023/12/20.
//

#pragma once
#include "doodle_core/metadata/scan_data_t.h"
#include <doodle_core/core/file_sys.h>
#include <doodle_core/doodle_core.h>
#include <doodle_core/metadata/assets.h>
#include <doodle_core/metadata/project.h>

#include <entt/entt.hpp>
#include <map>

namespace doodle::details {

class scan_category_data_t {
 private:
 public:
  using assets_type_enum = assets_type_enum;

  // uuid和文件路径的类
  struct uuid_path_t {
    uuid uuid_;
    FSys::path path_;
    // 文件修改时间
    FSys::file_time_type last_write_time_;
    // to_json
    friend void to_json(nlohmann::json& j, const uuid_path_t& p) {
      j["uuid"] = p.uuid_;
      j["path"] = p.path_;
    }

    // from_json
    friend void from_json(const nlohmann::json& j, uuid_path_t& p) {
      j.at("uuid").get_to(p.uuid_);
      j.at("path").get_to(p.path_);
    }
  };

  // ue文件
  uuid_path_t ue_file_;
  // rig文件
  uuid_path_t rig_file_;
  // 解算文件
  uuid_path_t solve_file_;

  // 这个是查找依据的根本路径, 必然不空, 可被打开
  FSys::path base_path_;

  // 项目根目录
  std::shared_ptr<project_helper::database_t> project_database_ptr;

  // 季数
  season season_;
  // 名称
  std::string name_;
  // 版本名称
  std::string version_name_;

  // 类型
  assets_type_enum assets_type_;
  // 编号
  std::string number_str_;

  // 文件hash(包含 ue(工程, 配置),rig,解算文件) 所有文件的文件名称, 大小, 修改时间 的hash
  std::string file_hash_;

  // to_json
  friend void to_json(nlohmann::json& j, const scan_category_data_t& p) {
    j["ue_file"]              = p.ue_file_;
    j["rig_file"]             = p.rig_file_;
    j["solve_file"]           = p.solve_file_;
    j["base_path"]            = p.base_path_;
    j["season"]               = p.season_;
    j["name"]                 = p.name_;
    j["version_name"]         = p.version_name_;
    j["assets_type"]          = p.assets_type_;
    j["number_str"]           = p.number_str_;
    j["file_hash"]            = p.file_hash_;
    j["project_database_ptr"] = p.project_database_ptr->uuid_id_;
  }
  // from_json
  friend void from_json(const nlohmann::json& j, scan_category_data_t& p) {
    j.at("ue_file").get_to(p.ue_file_);
    j.at("rig_file").get_to(p.rig_file_);
    j.at("solve_file").get_to(p.solve_file_);
    j.at("base_path").get_to(p.base_path_);
    j.at("season").get_to(p.season_);
    j.at("name").get_to(p.name_);
    j.at("version_name").get_to(p.version_name_);
    j.at("assets_type").get_to(p.assets_type_);
    j.at("number_str").get_to(p.number_str_);
    j.at("file_hash").get_to(p.file_hash_);
  }
};
using scan_category_data_ptr = std::shared_ptr<scan_category_data_t>;
class scan_category_t {
 public:
  logger_ptr logger_;
  std::shared_ptr<boost::asio::cancellation_state> cancellation_state_;
  scan_category_t() {}
  virtual ~scan_category_t() = default;
  virtual std::vector<scan_category_data_ptr> scan(
      const std::shared_ptr<project_helper::database_t>& in_root
  ) const = 0;

  virtual void scan_file_hash(const scan_category_data_ptr& in_data);

 private:
  // 计算hash
  std::string file_hash(const std::string& in_data);
};

NLOHMANN_JSON_SERIALIZE_ENUM(
    assets_type_enum,
    {
        {assets_type_enum::scene, "scene"},
        {assets_type_enum::prop, "prop"},
        {assets_type_enum::character, "character"},
        {assets_type_enum::rig, "rig"},
        {assets_type_enum::animation, "animation"},
        {assets_type_enum::vfx, "vfx"},
        {assets_type_enum::cfx, "cfx"},
        {assets_type_enum::other, "other"},
    }
);

}  // namespace doodle::details
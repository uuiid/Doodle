//
// Created by TD on 2023/12/20.
//

#include "prop_scan_category.h"

#include <doodle_core/metadata/assets_file.h>
#include <doodle_core/metadata/season.h>
namespace doodle::details {

std::vector<scan_category_data_ptr> prop_scan_category_t::scan(
    const std::shared_ptr<project_helper::database_t> &in_root
) const {
  const FSys::path l_prop_path_root = FSys::path{in_root->path_} / "6-moxing/Prop";
  const std::regex l_JD_regex{R"(JD(\d+)_(\d+))"};

  if (!FSys::exists(l_prop_path_root)) {
    logger_->log(log_loc(), level::err, "路径不存在:{}", l_prop_path_root);
    return {};
  }
  std::vector<std::shared_ptr<prop_scan_category_data_t>> l_out;
  std::smatch l_match{};

  for (const auto &l_s : FSys::directory_iterator{l_prop_path_root}) {  // 迭代一级目录
    const auto l_name_str_1 = l_s.path().filename().generic_string();
    if (!l_s.is_directory()) continue;
    if (!std::regex_match(l_name_str_1, l_match, l_JD_regex)) continue;

    season l_season{std::stoi(l_match[1].str())};
    const auto l_begin_episode = std::stoi(l_match[2].str());  // 获取开始集数
    const auto l_prop_path = l_s.path() / fmt::format("JD{:02}_{:02}_UE", l_season.p_int, l_begin_episode) / "Content" /
                             "Prop";  // 生成目标路径
    if (!FSys::exists(l_prop_path)) continue;
    if (!FSys::is_directory(l_prop_path)) continue;
    for (auto &&l_s2 : FSys::directory_iterator{l_prop_path}) {  // 迭代二级目录
      if (!l_s2.is_directory()) continue;
      const auto l_name_str       = l_s2.path().filename().generic_string();

      // 直接创建, 有空的, 但是也要在这里创建
      auto l_ptr                  = std::make_shared<prop_scan_category_data_t>();
      l_ptr->project_database_ptr = in_root;
      l_ptr->season_              = l_season;
      l_ptr->begin_episode_       = l_begin_episode;
      l_ptr->name_                = l_name_str;
      l_ptr->JD_path_             = l_s.path();
      l_ptr->base_path_           = l_s2.path();
      l_ptr->assets_type_         = scan_category_data_t::assets_type_enum::prop;
      l_ptr->file_type_.set_path("道具");
      l_out.emplace_back(l_ptr);

      auto l_mesh_path = l_s2.path() / "Mesh";  // 确认目标路径
      if (!FSys::exists(l_mesh_path)) continue;
      if (!FSys::is_directory(l_mesh_path)) continue;

      for (auto &&l_s3 : FSys::directory_iterator{l_mesh_path}) {
        if (l_s3.path().extension() != ".uasset") continue;
        auto l_stem = l_s3.path().stem().generic_string();

        if (!l_stem.starts_with(l_name_str))  // 检查文件名称和是否有不同的版本
          continue;

        auto l_version_str = l_stem.substr(l_name_str.size());
        if (l_version_str.starts_with("_")) {
          l_version_str = l_version_str.substr(1);
        }

        // 不同版本的显示
        if (!l_version_str.empty()) {
          l_ptr                       = std::make_shared<prop_scan_category_data_t>();
          l_ptr->project_database_ptr = in_root;
          l_ptr->season_              = l_season;
          l_ptr->begin_episode_       = l_begin_episode;
          l_ptr->name_                = l_name_str;
          l_ptr->JD_path_             = l_s.path();
          l_ptr->base_path_           = l_s2.path();
          l_ptr->assets_type_         = scan_category_data_t::assets_type_enum::prop;
          l_ptr->file_type_.set_path("道具");
          l_out.emplace_back(l_ptr);
          l_ptr->version_name_ = l_version_str;
        }

        l_ptr->ue_file_.path_            = l_s3.path();
        l_ptr->ue_file_.uuid_            = FSys::software_flag_file(l_s3.path());
        l_ptr->ue_file_.last_write_time_ = l_s3.last_write_time();

        logger_->log(log_loc(), level::info, "扫描到道具文件:{}", l_s3.path());  // 输出日志
      }
    }
  }

  // 继续扫描道具rig和maya文件
  for (auto &&l_ptr : l_out) {
    auto l_rig_path = l_ptr->JD_path_ / l_ptr->name_ / "Rig";
    std::string l_rig_file_name;
    if (l_ptr->version_name_.empty())
      l_rig_file_name = fmt::format("{}_rig", l_ptr->name_);
    else
      l_rig_file_name = fmt::format("{}_{}_rig", l_ptr->name_, l_ptr->version_name_);
    if (!FSys::exists(l_rig_path)) continue;
    if (!FSys::is_directory(l_rig_path)) continue;

    if ([&]() {
          for (auto &&l_file : FSys::directory_iterator{l_rig_path}) {
            auto l_file_stem = l_file.path().stem().generic_string();
            if (l_file_stem.starts_with(l_rig_file_name) && l_file.path().extension() == ".ma") {
              if (l_ptr->rig_file_.path_.empty())
                l_ptr->rig_file_.path_ = l_file.path();
              else {
                logger_->log(log_loc(), level::err, "rig文件存在多个:{}/{}***.ma", l_rig_path, l_rig_file_name);
                return true;
              }
            }
          }
          return false;
        }())
      continue;

    if (l_ptr->rig_file_.path_.empty()) {
      logger_->log(log_loc(), level::err, "rig文件不存在:{}/{}***.ma", l_rig_path, l_rig_file_name);
      continue;
    }

    l_ptr->rig_file_.uuid_            = FSys::software_flag_file(l_ptr->rig_file_.path_);
    l_ptr->rig_file_.last_write_time_ = FSys::last_write_time(l_ptr->rig_file_.path_);
    // 检查maya文件
    //
    //    auto l_maya_path                  = l_ptr->JD_path_ / l_ptr->name_;
    //    if (l_ptr->version_name_.empty())
    //      l_maya_path /= fmt::format("{}.ma", l_ptr->name_);
    //    else
    //      l_maya_path /= fmt::format("{}_{}.ma", l_ptr->name_, l_ptr->version_name_);
    //
    //    if (!FSys::exists(l_maya_path)) {
    //      logger_->log(log_loc(), level::err, "maya文件不存在:{}", l_maya_path);
    //      continue;
    //    }
    //    l_ptr->maya_file_.path_            = l_maya_path;
    //    l_ptr->maya_file_.uuid_            = FSys::software_flag_file(l_maya_path);
    //    l_ptr->maya_file_.last_write_time_ = FSys::last_write_time(l_maya_path);
  }

  return l_out | ranges::views::transform([](auto &&in_ptr) -> scan_category_data_ptr {
           in_ptr->rig_file_.path_.make_preferred();
           in_ptr->ue_file_.path_.make_preferred();
           in_ptr->solve_file_.path_.make_preferred();
           return in_ptr;
         }) |
         ranges::to_vector;
}

}  // namespace doodle::details
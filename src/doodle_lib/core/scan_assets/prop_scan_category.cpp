//
// Created by TD on 2023/12/20.
//

#include "prop_scan_category.h"

#include <doodle_core/metadata/assets_file.h>
#include <doodle_core/metadata/file_association.h>
#include <doodle_core/metadata/season.h>
namespace doodle::details {

std::vector<scan_category_data_ptr> prop_scan_category_t::scan(const project_root_t &in_root) const {
  const FSys::path l_prop_path_root = in_root.p_path / "6-moxing/Prop";
  const std::regex l_JD_regex{R"(JD(\d+)_(\d+))"};

  if (!FSys::exists(l_prop_path_root)) {
    logger_->log(log_loc(), level::err, "路径不存在:{}", l_prop_path_root);
    return {};
  }
  std::vector<std::shared_ptr<prop_scan_category_data_t>> l_out;
  std::smatch l_match{};

  for (const auto &l_s : FSys::directory_iterator{l_prop_path_root}) {  // 迭代一级目录
    const auto l_name_str_1 = l_s.path().filename().generic_string();
    if (l_s.is_directory() && std::regex_match(l_name_str_1, l_match, l_JD_regex)) {  // 检查一级目录
      season l_season{std::stoi(l_match[1].str())};
      const auto l_begin_episode = std::stoi(l_match[2].str());  // 获取开始集数

      const auto l_prop_path = l_s.path() / fmt::format("JD{:02}_{}_UE", l_season.p_int, l_begin_episode) / "Content" /
                               "Prop";  // 生成目标路径
      if (!FSys::exists(l_prop_path)) continue;
      for (auto &&l_s2 : FSys::directory_iterator{l_prop_path}) {  // 迭代二级目录
        if (l_s2.is_directory()) {
          const auto l_name_str = l_s2.path().filename().generic_string();

          auto l_mesh_path      = l_s2.path() / "Mesh";  // 确认目标路径
          if (!FSys::exists(l_mesh_path)) continue;
          if (!FSys::is_directory(l_mesh_path)) continue;

          for (auto &&l_s3 : FSys::directory_iterator{l_mesh_path}) {
            if (l_s3.path().extension() != ".uasset") continue;
            auto l_stem = l_s3.path().stem().generic_string();

            if (l_stem.starts_with(l_name_str)) {  // 检查文件名称和是否有不同的版本
              auto l_version_str = l_stem.substr(l_name_str.size());
              if (l_version_str.starts_with("_")) {
                l_version_str = l_version_str.substr(1);
              }

              auto l_ptr            = std::make_shared<prop_scan_category_data_t>();
              l_ptr->project_root_  = in_root;
              l_ptr->season_        = l_season;
              l_ptr->begin_episode_ = l_begin_episode;
              l_ptr->name_          = l_name_str;
              if (!l_version_str.empty()) l_ptr->version_name_ = l_version_str;
              l_ptr->JD_path_                  = l_s.path();
              l_ptr->ue_file_.path_            = l_s3.path();
              l_ptr->ue_file_.uuid_            = FSys::software_flag_file(l_s3.path());
              l_ptr->ue_file_.last_write_time_ = l_s3.last_write_time();
              l_ptr->assets_type_              = scan_category_data_t::assets_type_enum::prop;

              l_ptr->file_type_.set_path("道具");

              if (l_stem.find('_') != std::string::npos) {
                l_ptr->version_name_ = l_stem.substr(l_stem.find('_') + 1);
              }

              logger_->log(log_loc(), level::info, "扫描到道具文件:{}", l_s3.path());  // 输出日志
              l_out.emplace_back(l_ptr);
            }
          }
        }
      }
    }
  }

  // 继续扫描道具rig和maya文件
  for (auto &&l_ptr : l_out) {
    auto l_rig_path = l_ptr->JD_path_ / l_ptr->name_ / "Rig";
    std::string l_rig_file_name;
    if (l_ptr->version_name_.empty())
      l_rig_file_name = fmt::format("{}_rig_", l_ptr->name_);
    else
      l_rig_file_name = fmt::format("{}_{}_rig_", l_ptr->name_, l_ptr->version_name_);

    auto l_rig_files =
        ranges::make_subrange(FSys::directory_iterator{l_rig_path}, FSys::directory_iterator{}) |
        ranges::views::filter([&](auto &&i) -> bool {
          return i.path().filename().generic_string().starts_with(l_rig_file_name) && i.path().extension() == ".ma";
          ;
        }) |
        ranges::views::transform([](auto &&i) -> FSys::path { return i.path(); }) | ranges::to_vector;

    if (l_rig_files.empty()) {
      logger_->log(log_loc(), level::err, "rig文件夹中不存在:{} 符合 {}_***.ma 命名的文件 ", l_rig_path, l_ptr->name_);
      continue;
    }
    if (l_rig_files.size() > 1) {
      logger_->log(log_loc(), level::err, "符合rig 文件规则的文件数量大于1:{}", l_rig_files);
      continue;
    }

    l_ptr->rig_file_.path_            = l_rig_files.front();
    l_ptr->rig_file_.uuid_            = FSys::software_flag_file(l_rig_files.front());
    l_ptr->rig_file_.last_write_time_ = FSys::last_write_time(l_rig_files.front());
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

  return l_out | ranges::views::transform([](auto &&i) -> scan_category_data_ptr { return i; }) | ranges::to_vector;
}

}  // namespace doodle::details
//
// Created by TD on 2023/12/20.
//

#include "scene_scan_category.h"

#include <doodle_core/metadata/assets_file.h>
#include <doodle_core/metadata/season.h>

namespace doodle::details {

std::vector<scan_category_data_ptr> scene_scan_category_t::scan(
    const std::shared_ptr<project_minimal>& in_root
) const {
  const FSys::path l_scene_path = FSys::path{in_root->path_} / base_path_;
  const std::regex l_JD_regex{R"(JD(\d+)_(\d+))"};
  const std::regex l_BG_regex{R"(BG(\d+[a-zA-Z]\d*))"};

  if (std::error_code l_core_code{}; !FSys::exists(l_scene_path, l_core_code) || l_core_code) {
    logger_->log(log_loc(), level::err, "路径不存在:{}", l_scene_path);
    return {};
  }
  std::vector<std::shared_ptr<scene_scan_category_data_t>> l_out;
  std::smatch l_match{};

  // 第一次扫描目录, 获取基本的ue文件
  for (const auto& l_s : FSys::directory_iterator{l_scene_path}) {  // 迭代一级目录
    auto l_name_str = l_s.path().filename().generic_string();
    if (!(l_s.is_directory() && std::regex_match(l_name_str, l_match, l_JD_regex))) continue;  // 检查一级目录
    season l_season{std::stoi(l_match[1].str())};
    auto l_begin_episode = std::stoi(l_match[2].str());              // 获取开始集数
    for (const auto& l_s2 : FSys::directory_iterator{l_s.path()}) {  // 迭代二级目录
      auto l_name2_str = l_s2.path().filename().generic_string();
      if (!(l_s2.is_directory() && std::regex_match(l_name2_str, l_match, l_BG_regex))) continue;  // 检查二级目录
      auto l_number_str = l_match[1].str();                                                        // 获取编号
      if (cancellation_state_ && cancellation_state_->cancelled() != boost::asio::cancellation_type::none) return {};

      for (auto&& l_s3 : FSys::directory_iterator{l_s2.path()}) {  // 迭代三级目录
        if (!l_s3.is_directory()) continue;
        if (!FSys::exists(l_s3.path() / "Content")) continue;

        // 直接创建, 有空的, 但是也要在这里创建
        auto l_ptr                  = std::make_shared<scene_scan_category_data_t>();
        l_ptr->season_              = l_season;
        l_ptr->project_database_ptr = in_root;
        l_ptr->begin_episode_       = l_begin_episode;
        l_ptr->name_                = l_s3.path().filename().generic_string();
        l_ptr->BG_path_             = l_s2.path();
        l_ptr->base_path_           = l_s3.path();
        l_ptr->assets_type_         = scan_category_data_t::assets_type_enum::scene;
        l_ptr->number_str_          = l_number_str;

        l_out.emplace_back(l_ptr);

        auto l_dis_path = l_s3.path() / "Content" / l_s3.path().filename() / "Map";  // 确认目标路径
        if (!FSys::exists(l_dis_path)) continue;
        for (auto&& l_s4 : FSys::directory_iterator{l_dis_path}) {                     // 迭代四级目录
          if (l_s4.is_regular_file() && l_s4.path().extension() != ".umap") continue;  // 确认后缀名称
          auto l_stem = l_s4.path().stem().generic_string();
          if (!l_stem.starts_with(l_s3.path().filename().generic_string())) continue;

          auto l_version_str = l_stem.substr(l_s3.path().filename().generic_string().size());
          if (l_version_str.starts_with("_")) {
            l_version_str = l_version_str.substr(1);
          }
          if (!l_version_str.empty()) {
            l_ptr                       = std::make_shared<scene_scan_category_data_t>();
            l_ptr->season_              = l_season;
            l_ptr->project_database_ptr = in_root;
            l_ptr->begin_episode_       = l_begin_episode;
            l_ptr->name_                = l_s3.path().filename().generic_string();
            l_ptr->BG_path_             = l_s2.path();
            l_ptr->base_path_           = l_s3.path();
            l_ptr->assets_type_         = scan_category_data_t::assets_type_enum::scene;
            l_ptr->number_str_          = l_number_str;
            l_out.emplace_back(l_ptr);
            l_ptr->version_name_ = l_version_str;
          }

          l_ptr->ue_file_.path_            = l_s4.path();
          l_ptr->ue_file_.uuid_            = FSys::software_flag_file(l_s4.path());
          l_ptr->ue_file_.last_write_time_ = l_s4.last_write_time();
          logger_->log(log_loc(), level::info, "扫描到场景文件:{}", l_s4.path());
        }
      }
    }
  }
  // 开始确认maya文件
  for (auto&& l_ptr : l_out) {
    if (cancellation_state_ && cancellation_state_->cancelled() != boost::asio::cancellation_type::none) return {};

    auto l_rig_path = l_ptr->BG_path_ / "Mod";
    if (l_ptr->version_name_.empty())
      l_rig_path /= fmt::format("{}_Low.ma", l_ptr->name_);
    else
      l_rig_path /= fmt::format("{}_{}_Low.ma", l_ptr->name_, l_ptr->version_name_);

    if (!FSys::exists(l_rig_path)) {
      logger_->log(log_loc(), level::err, "rig文件不存在:{}", l_rig_path);
      continue;
    }
    l_ptr->rig_file_.path_            = l_rig_path;
    l_ptr->rig_file_.uuid_            = FSys::software_flag_file(l_rig_path);
    l_ptr->rig_file_.last_write_time_ = FSys::last_write_time(l_rig_path);
  }

  return l_out | ranges::views::transform([](auto&& in_ptr) -> scan_category_data_ptr {
           in_ptr->rig_file_.path_.make_preferred();
           in_ptr->ue_file_.path_.make_preferred();
           in_ptr->solve_file_.path_.make_preferred();
           return in_ptr;
         }) |
         ranges::to<std::vector>();
}

}  // namespace doodle::details
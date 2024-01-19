//
// Created by TD on 2023/12/20.
//

#include "character_scan_category.h"
namespace doodle::details {

std::vector<scan_category_data_ptr> character_scan_category_t::scan(const project_root_t &in_root) const {
  const FSys::path l_character_path_root = in_root.p_path / "6-moxing/Ch";
  const std::regex l_JD_regex{R"(JD(\d+)_(\d+))"};
  const std::regex l_Ch_regex{R"(Ch(\d+[A-Z]))"};
  std::regex l_Ue_regex{R"((\w+)_UE\d)"};

  if (!FSys::exists(l_character_path_root)) {
    logger_->log(log_loc(), level::err, "路径不存在:{}", l_character_path_root);
    return {};
  }
  std::vector<std::shared_ptr<character_scan_category_data_t>> l_out;
  std::smatch l_match{};

  for (auto &&l_s : FSys::directory_iterator{l_character_path_root}) {
    auto l_Jd_path = l_s.path();
    auto l_Jd_name = l_Jd_path.filename().generic_string();
    if (!FSys::is_directory(l_Jd_path)) continue;
    if (!std::regex_match(l_Jd_name, l_match, l_JD_regex)) continue;
    season l_season{std::stoi(l_match[1].str())};
    auto l_begin_episode = std::stoi(l_match[2].str());
    for (auto &&l_s2 : FSys::directory_iterator{l_Jd_path}) {
      if (!FSys::is_directory(l_s2.path())) continue;
      auto l_ChNum_path = l_s2.path();
      auto l_ChNum_name = l_ChNum_path.filename().generic_string();
      if (!std::regex_match(l_ChNum_name, l_match, l_Ch_regex)) continue;

      auto l_number_str       = l_match[1].str();
      auto l_Sk_ch_number_str = fmt::format("SK_Ch{}", l_number_str);
      for (auto &&l_s3 : FSys::directory_iterator{l_ChNum_path}) {  // 迭代Ue项目路径
        if (!FSys::is_directory(l_s3.path())) continue;
        auto l_ch_name_ue_path = l_s3.path();
        auto l_ch_name_ue_name = l_ch_name_ue_path.filename().generic_string();
        if (!std::regex_match(l_ch_name_ue_name, l_match, l_Ue_regex)) continue;
        auto l_ch_name          = l_match[1].str();
        auto l_ch_ue_asset_path = l_ch_name_ue_path / "Content" / "Character" / l_ch_name / "Meshs";

        if (!FSys::exists(l_ch_ue_asset_path)) continue;
        if (!FSys::is_directory(l_ch_ue_asset_path)) continue;

        for (auto &&l_s4 : FSys::directory_iterator{l_ch_ue_asset_path}) {
          if (!FSys::is_regular_file(l_s4.path())) continue;
          auto l_ch_ue_asset_stem = l_s4.path().stem().generic_string();
          if (l_s4.path().extension() != ".uasset") continue;
          if (!l_ch_ue_asset_stem.starts_with(l_Sk_ch_number_str)) continue;

          auto l_version_str = l_ch_ue_asset_stem.substr(l_ch_name.size());
          if (l_version_str.starts_with("_")) l_version_str = l_version_str.substr(1);

          auto l_ptr                       = std::make_shared<character_scan_category_data_t>();
          l_ptr->project_root_             = in_root;
          l_ptr->season_                   = l_season;
          l_ptr->name_                     = l_ch_name;
          l_ptr->Ch_path_                  = l_ChNum_path;
          l_ptr->begin_episode_            = l_begin_episode;
          l_ptr->ue_file_.path_            = l_s4.path();
          l_ptr->ue_file_.uuid_            = FSys::software_flag_file(l_s4.path());
          l_ptr->ue_file_.last_write_time_ = l_s4.last_write_time();
          l_ptr->number_str_               = l_number_str;
          if (!l_version_str.empty()) l_ptr->version_name_ = l_version_str;
          l_ptr->file_type_.set_path("角色");
          l_ptr->assets_type_ = scan_category_data_t::assets_type_enum::character;
          logger_->log(log_loc(), level::info, "扫描到角色文件:{}", l_s4.path());
          l_out.emplace_back(l_ptr);
        }
      }
    }
  }
  // 添加rig文件
  for (auto &&l_ptr : l_out) {
    auto l_rig_path = l_ptr->Ch_path_ / "Rig";
    auto l_rig_name = fmt::format("Ch{}_rig_", l_ptr->number_str_);
    if (!FSys::exists(l_rig_path)) continue;
    if (!FSys::is_directory(l_rig_path)) continue;

    auto l_files =
        ranges::make_subrange(FSys::directory_iterator{l_rig_path}, FSys::directory_iterator{}) |
        ranges::views::filter([&l_rig_name](const auto &in_path) {
          return in_path.path().stem().generic_string().starts_with(l_rig_name) && in_path.path().extension() == ".ma";
        }) |
        ranges::to_vector;
    if (l_files.empty()) {
      logger_->log(log_loc(), level::err, "rig文件不存在:{}/{}***.ma", l_rig_path, l_rig_name);
      continue;
    }
    if (l_files.size() > 1) {
      logger_->log(log_loc(), level::err, "rig文件存在多个:{}/{}***.ma", l_rig_path, l_rig_name);
      continue;
    }
    l_rig_path = l_files[0].path();

    if (!FSys::exists(l_rig_path)) {
      logger_->log(log_loc(), level::err, "rig文件不存在:{}", l_rig_path);
      continue;
    }
    l_ptr->rig_file_.path_            = l_rig_path;
    l_ptr->rig_file_.uuid_            = FSys::software_flag_file(l_rig_path);
    l_ptr->rig_file_.last_write_time_ = FSys::last_write_time(l_rig_path);
    logger_->log(log_loc(), level::info, "扫描到rig文件:{}", l_rig_path);
  }

  return l_out | ranges::views::transform([](const auto &in_ptr) -> scan_category_data_ptr { return in_ptr; }) |
         ranges::to_vector;
}

}  // namespace doodle::details
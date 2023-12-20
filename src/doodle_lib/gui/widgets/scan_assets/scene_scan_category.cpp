//
// Created by TD on 2023/12/20.
//

#include "scene_scan_category.h"

#include <doodle_core/metadata/assets_file.h>
#include <doodle_core/metadata/file_association.h>
#include <doodle_core/metadata/season.h>

namespace doodle::gui::details {

std::vector<entt::handle> scene_scan_category_t::scan(const project_root_t& in_root) const {
  const FSys::path l_scene_path = in_root.path_ / "6-moxing/BG";
  const std::regex l_JD_regex{R"(JD(\d+)_(\d+))"};
  const std::regex l_BG_regex{R"(BG(\d+[a-zA-Z]\d*))"};

  if (!FSys::exists(l_scene_path)) {
    logger_->log(log_loc(), level::err, "路径不存在:{}", l_scene_path);
    return {};
  }
  std::vector<entt::handle> l_out;
  std::smatch l_match{};

  for (const auto& l_s : FSys::directory_iterator{l_scene_path}) {  // 迭代一级目录
    auto l_name_str = l_s.path().filename().generic_string();
    if (l_s.is_directory() && std::regex_match(l_name_str, l_match, l_JD_regex)) {  // 检查一级目录
      season l_season{std::stoi(l_match[1].str())};
      capture_data_t l_capture_data{std::stoi(l_match[2].str()), ""};
      for (const auto& l_s2 : FSys::directory_iterator{l_s.path()}) {  // 迭代二级目录
        auto l_name2_str = l_s2.path().filename().generic_string();
        if (l_s2.is_directory() && std::regex_match(l_name2_str, l_match, l_BG_regex)) {  // 检查二级目录
          l_capture_data.number_str_ = l_match[1].str();
          for (auto&& l_s3 : FSys::directory_iterator{l_s2.path()}) {  // 迭代三级目录
            if (l_s3.is_directory()) {
              auto l_dis_path = l_s3.path() / "Content" / l_s3.path().filename() / "Map";  // 确认目标路径
              if (!FSys::exists(l_dis_path)) continue;
              for (auto&& l_s4 : FSys::directory_iterator{l_dis_path}) {             // 迭代四级目录
                if (l_s4.is_regular_file() && l_s4.path().extension() == ".umap") {  // 确认后缀名称
                  auto l_stem = l_s4.path().stem().generic_string();

                  if (l_stem == l_s3.path().filename().generic_string() ||  // 检查文件名和文件格式
                      (l_stem.starts_with(l_s3.path().filename().generic_string()) &&
                       std::count(l_stem.begin(), l_stem.end(), '_') == 1)) {
                    auto l_uuid = FSys::software_flag_file(l_s4.path());
                    entt::handle l_handle;
                    auto l_capture_data_1 = l_capture_data;
                    if (uuid_map_entt_->contains(l_uuid)) {
                      l_handle = l_out.emplace_back(*g_reg(), uuid_map_entt_->at(l_uuid));
                    } else {
                      assets_file l_assets_file{l_s4.path(), l_s3.path().filename().generic_string(), 0};

                      l_handle = entt::handle{*g_reg(), g_reg()->create()};
                      l_handle.emplace<season>(l_season);
                      l_handle.emplace<assets_file>(std::move(l_assets_file));

                      l_out.push_back(l_handle);
                    }
                    logger_->log(log_loc(), level::info, "扫描到场景文件:{}", l_s4.path());
                    l_capture_data_1.version_str_ = l_stem.substr(l_stem.find('_') + 1);
                    l_handle.emplace_or_replace<capture_data_t>(l_capture_data_1);
                  }
                }
              }
            }
          }
        }
      }
    }
  }
  return l_out;
}

std::vector<entt::handle> scene_scan_category_t::check_path(const project_root_t& in_root, entt::handle& in_path)
    const {
  const FSys::path l_scene_path = in_root.path_ / "6-moxing/BG";

  if (!in_path.any_of<season, assets_file>()) return {};
  if (!FSys::exists(l_scene_path)) return {};

  auto& l_season  = in_path.get<season>();
  auto& l_assets  = in_path.get<assets_file>();
  auto& l_data    = in_path.get<capture_data_t>();

  // 检查rig文件
  auto l_rig_path = l_scene_path / fmt::format("JD{:02}_{}", l_season.get_season(), l_data.begin_episode_) /
                    fmt::format("BG{}", l_data.number_str_) / "Mod" /
                    fmt::format("{}_{}_Low.ma", l_assets.name_attr(), l_data.version_str_);
  if (!FSys::exists(l_rig_path)) {
    logger_->log(log_loc(), level::err, "rig文件不存在:{}", l_rig_path);
    return {};
  }

  auto l_rig_assets_file = assets_file{l_rig_path, l_assets.name_attr(), 0};
  entt::handle l_rig_handle{};
  entt::handle l_file_association_handle{};
  auto l_uuid = FSys::software_flag_file(l_rig_path);
  if (uuid_map_entt_->contains(l_uuid)) {
    l_rig_handle = entt::handle{*g_reg(), uuid_map_entt_->at(l_uuid)};

  } else {
    l_rig_handle = entt::handle{*g_reg(), g_reg()->create()};
    l_rig_handle.emplace<season>(l_season);
    l_rig_handle.emplace<assets_file>(std::move(l_rig_assets_file));
  }

  if (in_path.any_of<file_association_ref>()) {
    l_file_association_handle = in_path.get<file_association_ref>();
  } else if (l_rig_handle.any_of<file_association_ref>()) {
    l_file_association_handle = l_rig_handle.get<file_association_ref>();
  } else {
    l_file_association_handle = entt::handle{*g_reg(), g_reg()->create()};
  }

  // 创建联系
  file_association l_file_association{};
  l_file_association.ue_file       = in_path;
  l_file_association.maya_file     = l_rig_handle;
  l_file_association.maya_rig_file = l_rig_handle;
  l_file_association_handle.emplace_or_replace<file_association>(std::move(l_file_association));
  l_rig_handle.emplace_or_replace<file_association_ref>(l_file_association_handle);
  in_path.emplace_or_replace<file_association_ref>(l_file_association_handle);
  return {l_rig_handle};
}

}  // namespace doodle::gui::details
//
// Created by TD on 2023/12/20.
//

#include "prop_scan_category.h"

#include <doodle_core/metadata/assets_file.h>
#include <doodle_core/metadata/file_association.h>
#include <doodle_core/metadata/season.h>
namespace doodle::gui::details {

std::vector<entt::handle> prop_scan_category_t::scan(const project_root_t &in_root) const {
  const FSys::path l_prop_path = in_root.path_ / "6-moxing/Prop";
  const std::regex l_JD_regex{R"(JD(\d+)_(\d+))"};

  if (!FSys::exists(l_prop_path)) {
    return {};
  }
  std::vector<entt::handle> l_out;
  std::smatch l_match{};

  for (const auto &l_s : FSys::directory_iterator{l_prop_path}) {  // 迭代一级目录
    auto l_name_str = l_s.path().filename().generic_string();
    if (l_s.is_directory() && std::regex_match(l_name_str, l_match, l_JD_regex)) {  // 检查一级目录
      season l_season{std::stoi(l_match[1].str())};
      capture_data_t l_capture_data{std::stoi(l_match[2].str()), ""};

      auto l_prop_path = l_s.path() / fmt::format("JD{:02}_{}_UE", l_season.p_int, l_capture_data.begin_episode_) /
                         "Content" / "Prop";  // 生成目标路径
      if (!FSys::exists(l_prop_path)) continue;
      for (auto &&l_s2 : FSys::directory_iterator{l_prop_path}) {  // 迭代二级目录
        if (l_s2.is_directory()) {
          auto l_capture_data_1      = l_capture_data;
          l_capture_data_1.name_str_ = l_s2.path().filename().generic_string();

          auto l_mesh_path           = l_s2.path() / "Mesh";  // 确认目标路径
          if (!FSys::exists(l_mesh_path)) continue;
          if (!FSys::is_directory(l_mesh_path)) continue;

          for (auto &&l_s3 : FSys::directory_iterator{l_mesh_path}) {
            if (l_s3.path().extension() != ".uasset") continue;
            auto l_stem = l_s3.path().stem().generic_string();

            if (l_stem == l_capture_data_1.name_str_ ||
                (l_stem.starts_with(l_capture_data_1.name_str_) && std::count(l_stem.begin(), l_stem.end(), '_') == 1
                )) {  // 检查文件名称和是否有不同的版本

              if (l_stem.find('_') != std::string::npos) {
                l_capture_data_1.version_str_ = l_stem.substr(l_stem.find('_') + 1);
              }

              auto l_uuid = FSys::software_flag_file(l_s2.path());
              entt::handle l_handle;
              if (uuid_map_entt_->contains(l_uuid)) {
                l_handle = l_out.emplace_back(*g_reg(), uuid_map_entt_->at(l_uuid));
              } else {
                assets_file l_assets_file{l_s3.path(), l_stem, 0};

                l_handle = entt::handle{*g_reg(), g_reg()->create()};
                l_handle.emplace<season>(l_season);
                l_handle.emplace<assets_file>(std::move(l_assets_file));
                l_out.push_back(l_handle);
              }
              l_handle.emplace_or_replace<capture_data_t>(l_capture_data_1);
            }
          }
        }
      }
    }
  }
  return l_out;
}

std::vector<entt::handle> prop_scan_category_t::check_path(const project_root_t &in_root, entt::handle &in_path) const {
  // 检查maya文件
  const FSys::path l_prop_path = in_root.path_ / "6-moxing/Prop";
  const std::regex l_JD_regex{R"(JD(\d+)_(\d+))"};

  std::vector<entt::handle> l_out;

  if (!FSys::exists(l_prop_path)) {
    return l_out;
  }

  auto &l_season   = in_path.get<season>();
  auto &l_assets   = in_path.get<assets_file>();
  auto &l_data     = in_path.get<capture_data_t>();
  auto l_maya_file = l_prop_path / fmt::format("JD{:02}_{}", l_season.p_int, l_data.begin_episode_) / l_data.name_str_ /
                     l_data.name_str_;
  if (!l_data.version_str_.empty()) l_maya_file += fmt::format("_{}", l_data.version_str_);
  l_maya_file += ".ma";

  if (!FSys::exists(l_maya_file)) return l_out;

  auto l_uuid = FSys::software_flag_file(l_maya_file);
  // 创建对于句柄
  entt::handle l_maya_handle;
  entt::handle l_file_association_handle{};  // 文件关联句柄
  file_association l_file_association{};
  if (uuid_map_entt_->contains(l_uuid)) {
    l_maya_handle = entt::handle{*g_reg(), uuid_map_entt_->at(l_uuid)};
  } else {
    l_maya_handle = entt::handle{*g_reg(), g_reg()->create()};
    l_maya_handle.emplace<assets_file>(l_maya_file, l_assets.name_attr(), 0);
    l_maya_handle.emplace<season>(l_season);
  }
  // 检查文件关联
  if (in_path.any_of<file_association_ref>()) {
    l_file_association_handle = in_path.get<file_association_ref>();
  } else if (l_maya_handle.any_of<file_association_ref>()) {
    l_file_association_handle = l_maya_handle.get<file_association_ref>();
  } else {
    l_file_association_handle = entt::handle{*g_reg(), g_reg()->create()};
    l_file_association_handle.emplace<file_association>();
  }

  l_file_association.ue_file   = in_path;
  l_file_association.maya_file = l_maya_handle;
  l_maya_handle.emplace_or_replace<file_association_ref>(l_file_association_handle);
  in_path.emplace_or_replace<file_association_ref>(l_file_association_handle);

  l_out.emplace_back(l_maya_handle);

  // 检查rig文件
  auto l_maya_rig_file_path =
      l_prop_path / fmt::format("JD{:02}_{}", l_season.p_int, l_data.begin_episode_) / l_data.name_str_ / "Rig";
  auto l_maya_rig_file_name = l_data.name_str_;
  if (!l_data.version_str_.empty()) l_maya_rig_file_name += fmt::format("_{}", l_data.version_str_);
  l_maya_rig_file_name += "_rig_";
  auto l_files = ranges::make_subrange(FSys::directory_iterator{l_maya_rig_file_path}, FSys::directory_iterator{}) |
                 ranges::views::filter([&l_maya_rig_file_name](auto &&i) -> bool {
                   return i.path().filename().generic_string().starts_with(l_maya_rig_file_name);
                 }) |
                 ranges::views::transform([](auto &&i) -> FSys::path { return i.path(); }) | ranges::to_vector;

  if (l_files.empty()) return l_out;
  if (l_files.size() > 1) {
    return l_out;
  }
  auto l_rig_file = l_files.front();

  // 创建maya rig对应句柄
  entt::handle l_maya_rig_handle;
  if (uuid_map_entt_->contains(l_uuid)) {
    l_maya_rig_handle = entt::handle{*g_reg(), uuid_map_entt_->at(l_uuid)};
  } else {
    l_maya_rig_handle = entt::handle{*g_reg(), g_reg()->create()};
    l_maya_rig_handle.emplace<assets_file>(l_rig_file, l_assets.name_attr(), 0);
    l_maya_rig_handle.emplace<season>(l_season);
  }
  l_out.emplace_back(l_maya_rig_handle);
  // 检查文件关联
  l_file_association.maya_rig_file = l_maya_rig_handle;
  l_maya_rig_handle.emplace_or_replace<file_association_ref>(l_file_association_handle);
  l_file_association_handle.emplace_or_replace<file_association>(std::move(l_file_association));
  return l_out;
}

}  // namespace doodle::gui::details
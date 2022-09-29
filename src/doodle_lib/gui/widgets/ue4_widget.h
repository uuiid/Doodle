//
// Created by TD on 2022/4/1.
//

#pragma once

#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_core/metadata/export_file_info.h>
#include <doodle_core/core/init_register.h>
#include <doodle_app/gui/base/base_window.h>

namespace doodle::gui {

namespace ue4_widget_n {
class ue4_import_data {
 public:
  ue4_import_data();
  explicit ue4_import_data(const export_file_info& in_info);
  std::string import_file_path{};
  std::string import_file_save_dir{};
  export_file_info::export_type import_type{};
  std::string fbx_skeleton_file_name{};
  std::uint64_t start_frame{};
  std::uint64_t end_frame{};

  [[nodiscard]] std::string find_ue4_skin(const FSys::path& in_ref_file, const FSys::path& in_ue4_content_dir, const std::string& in_regex, const std::string& in_fmt) const;

  [[nodiscard]] std::string set_save_dir(
      const entt::handle& in_handle
  ) const;

  /**
   * @brief 传入一个读取出 json_doodle 的文件路径
   * @param in_path json_doodle 的文件路径
   */
  void redirect_path(const FSys::path& in_path);

 private:
  friend void to_json(nlohmann::json& j, const ue4_import_data& p);
  friend void from_json(const nlohmann::json& j, ue4_import_data& p);
};
class ue4_import_group {
 public:
  std::uint64_t start_frame;
  std::uint64_t end_frame;
  std::string world_path;
  std::string level_path;
  std::vector<ue4_import_data> groups;

  [[nodiscard]] std::string set_level_dir(
      const entt::handle& in_handle,
      const std::string& in_e = {}
  ) const;

 private:
  friend void to_json(nlohmann::json& j, const ue4_import_group& p);
  friend void from_json(const nlohmann::json& j, ue4_import_group& p);
};

}  // namespace ue4_widget_n

class DOODLELIB_API ue4_widget
    : public base_windows<
          dear::Begin,
          ue4_widget> {
  class impl;
  std::unique_ptr<impl> p_i;

  void import_ue4_prj();
  void accept_handle(const std::vector<entt::handle>& in_list);
  void plan_file_path(const FSys::path& in_path);

 public:
  ue4_widget();
  ~ue4_widget() override;
  constexpr static std::string_view name{gui::config::menu_w::ue4_widget};
  const std::string& title() const override;
  void init();
  void render();
};

}  // namespace doodle::gui

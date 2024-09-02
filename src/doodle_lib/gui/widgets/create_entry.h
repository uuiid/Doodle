//
// Created by td_main on 2023/3/30.
//

#pragma once

#include <doodle_lib/doodle_lib_fwd.h>

#include "entt/entity/fwd.hpp"
#include <functional>
#include <memory>
#include <vector>
namespace doodle::gui {

class create_entry {
 public:
  class init_args {
    friend class create_entry;
    std::vector<FSys::path> paths_{};
    std::function<void(const std::vector<entt::handle>&)> fun_{};

   public:
    init_args() = default;

    inline init_args& set_create_call(const std::function<void(const std::vector<entt::handle>&)>& in_function) {
      fun_ = in_function;
      return *this;
    }
    inline init_args& set_paths(const std::vector<FSys::path>& in_paths) {
      paths_ = in_paths;
      return *this;
    };
  };

 private:
  struct ass_type_t {
    entt::handle handle_;
    std::string asset_type_;
  };
  std::shared_ptr<init_args> args_{};
  /// 重复的路径
  std::vector<FSys::path> duplicate_paths_{};
  std::vector<entt::handle> duplicate_handles_{};
  std::vector<ass_type_t> ass_type_list_{};
  enum class sources_file_type { project_open, project_import, other_files };
  sources_file_type sources_file_type_{sources_file_type::project_open};

  void create_ass_type_list();

  void switch_sources_file();
  void find_icon(const entt::handle& in_handle, const FSys::path& in_path) const;

  void render_project_open_files();
  void render_project_import_files();
  void render_other_files();

  void find_duplicate_file();

 public:
  explicit create_entry(const init_args& in_args) : args_(std::make_shared<init_args>(in_args)) {
    switch_sources_file();
  }

  constexpr static std::string_view name{gui::config::menu_w::create_entry_};
  static constexpr std::array<float, 2> sizexy{640, 360};
  bool render();
};

}  // namespace doodle::gui

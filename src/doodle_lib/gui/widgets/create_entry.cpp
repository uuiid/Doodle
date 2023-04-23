//
// Created by td_main on 2023/3/30.
//

#include "create_entry.h"

#include "doodle_core/core/core_help_impl.h"
#include "doodle_core/metadata/assets_file.h"
#include "doodle_core/metadata/metadata.h"
#include <doodle_core/metadata/time_point_wrap.h>

#include <doodle_lib/core/image_loader.h>

#include "entt/entity/fwd.hpp"
#include "imgui.h"
#include "range/v3/view/transform.hpp"

namespace doodle::gui {
bool create_entry::render() {
  args_->fun_(
      args_->paths_ | ranges::views::transform([=](const FSys::path &in_path) -> entt::handle {
        auto l_ent      = entt::handle{*g_reg(), g_reg()->create()};
        auto l_prj_path = g_reg()->ctx().get<project>().p_path;
        /// \brief 这里使用 lexically_proximate 防止相对路径失败
        auto l_path     = in_path.lexically_proximate(l_prj_path);

        season::analysis_static(l_ent, in_path);
        episodes::analysis_static(l_ent, in_path);
        shot::analysis_static(l_ent, in_path);
        episodes::conjecture_season(l_ent);

        if (FSys::exists(in_path)) {
          l_ent.emplace_or_replace<time_point_wrap>(FSys::last_write_time_point(in_path));
        }
        find_icon(l_ent, in_path);
        l_ent.emplace<database>();
        l_ent.emplace<assets_file>(l_path);
        return l_ent;
      }) |
      ranges::to_vector
  );
  ImGui::CloseCurrentPopup();
  return true;
}
void create_entry::find_icon(const entt::handle &in_handle, const FSys::path &in_path) const {
  image_loader l_image_load{};

  auto &l_config = g_reg()->ctx().get<project_config::base_config>();
  std::regex l_regex{l_config.find_icon_regex};
  FSys::path l_path{in_path};
  if (FSys::is_regular_file(l_path) && l_config.match_icon_extensions(l_path)) {
    l_image_load.save(in_handle, l_path);
    return;
  } else if (FSys::is_regular_file(l_path))
    l_path = in_path.parent_path();
  else
    l_path = in_path;

  if (FSys::is_directory(l_path)) {
    auto k_imghe_path = ranges::find_if(
        ranges::make_subrange(FSys::directory_iterator{l_path}, FSys::directory_iterator{}),
        [&](const FSys::path &in_file) {
          return l_config.match_icon_extensions(in_file) &&
                 std::regex_search(in_file.filename().generic_string(), l_regex);
        }
    );
    if (k_imghe_path != FSys::directory_iterator{}) {
      l_image_load.save(in_handle, k_imghe_path->path());
    }
  }
}
}  // namespace doodle::gui
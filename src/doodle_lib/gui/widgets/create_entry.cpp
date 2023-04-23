//
// Created by td_main on 2023/3/30.
//

#include "create_entry.h"

#include "doodle_core/core/core_help_impl.h"
#include "doodle_core/metadata/assets_file.h"
#include "doodle_core/metadata/metadata.h"

#include "entt/entity/fwd.hpp"
#include "range/v3/view/transform.hpp"

namespace doodle::gui {
bool create_entry::render() {
  args_->fun_(
      args_->paths_ | ranges::views::transform([=](const FSys::path &path) -> entt::handle {
        auto l_ent = entt::handle{*g_reg(), g_reg()->create()};
        l_ent.emplace<database>();
        l_ent.emplace<assets_file>().path_attr(path);
        return l_ent;
      }) |
      ranges::to_vector
  );

  return true;
}
}  // namespace doodle::gui
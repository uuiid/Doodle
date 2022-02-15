//
// Created by TD on 2022/2/15.
//

#include "database_edit.h"
#include <metadata/metadata.h>
#include <lib_warp/imgui_warp.h>

namespace doodle::gui {
class database_edit::impl {
 public:
  std::string show_text;
};
void database_edit::init_(const entt::handle& in) {
  auto&& l_item = in.get_or_emplace<database>();

  std::string l_status{};
  switch (l_item.status_) {
    case database::status::is_sync:
      l_status = "同步状态";
      break;
    case database::status::need_save:
      l_status = "需要保存";
      break;
    case database::status::need_load:
      l_status = "需要加载";
      break;
    case database::status::need_delete:
      l_status = "需要删除";
      break;
    default:
      l_status = "需要保存";
      break;
  }
  fmt::format(R"(数据 id : {}
数据状态 {}
)",
              l_item.get_id(), l_status);
}
void database_edit::save_(const entt::handle& in) const {
  if (in.all_of<database>()) {
    in.patch<database>(database::save);
  }
}
database_edit::database_edit()
    : p_i(std::make_unique<impl>()) {
}
database_edit::~database_edit() = default;
void database_edit::render(const entt::handle& in) {
  dear::Text(p_i->show_text);
}
}  // namespace doodle::gui

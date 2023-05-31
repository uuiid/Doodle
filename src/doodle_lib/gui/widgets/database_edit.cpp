//
// Created by TD on 2022/2/15.
//

#include "database_edit.h"

#include <doodle_core/metadata/metadata.h>

#include <doodle_app/lib_warp/imgui_warp.h>

namespace doodle::gui {
class database_edit::impl {
 public:
  std::string show_text;
  std::uint64_t id{};
  std::string status;
  std::string is_edit;

  std::vector<boost::signals2::scoped_connection> scoped_connection_;
};
void database_edit::init_(const entt::handle& in) {
  auto&& l_item = in.get_or_emplace<database>();

  std::string l_status{};
  if (in.any_of<data_status_save, data_status_delete>())
    p_i->status = "需要保存";
  else
    p_i->status = "已同步";

  p_i->id      = l_item.get_id();
  p_i->is_edit = data_->is_modify ? "(已编辑)"s : ""s;

  this->format_();
}
void database_edit::save_(const entt::handle& in) const {
  DOODLE_CHICK(in.all_of<database>(), doodle_error{"缺失数据库组件"});
}
database_edit::database_edit() : p_i(std::make_unique<impl>()) {}
database_edit::~database_edit() = default;
void database_edit::render(const entt::handle& in) { dear::Text(p_i->show_text); }
void database_edit::link_sig(const std::unique_ptr<edit_interface>& in_unique_ptr) {
  p_i->scoped_connection_.emplace_back(in_unique_ptr->data_->edited.connect([this]() {
    this->data_->is_modify = true;
    this->p_i->is_edit     = "(已编辑)"s;
    this->format_();
  }));
}
void database_edit::format_() {
  p_i->show_text = fmt::format(
      R"(数据 id : {}
数据状态 {} {}
)",
      p_i->id, p_i->status, p_i->is_edit
  );
}
}  // namespace doodle::gui

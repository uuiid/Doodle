
//
// Created by TD on 2021/9/23.
//

#include "edit_widget.h"

#include <doodle_core/core/core_sig.h>
#include <doodle_core/core/init_register.h>
#include <doodle_core/metadata/assets_file.h>
#include <doodle_core/metadata/image_icon.h>
#include <doodle_core/metadata/importance.h>
#include <doodle_core/metadata/metadata.h>
#include <doodle_core/metadata/metadata_cpp.h>

#include <doodle_app/gui/base/ref_base.h>

#include <doodle_lib/core/image_loader.h>
#include <doodle_lib/gui/widgets/assets_file_widgets.h>
#include <doodle_lib/gui/widgets/assets_filter_widget.h>
#include <doodle_lib/gui/widgets/derail/all_user_combox.h>
#include <doodle_lib/gui/widgets/derail/assets_file_edit.h>
#include <doodle_lib/gui/widgets/derail/command_edit.h>
#include <doodle_lib/gui/widgets/derail/episodes_edit.h>
#include <doodle_lib/gui/widgets/derail/importance_edit.h>
#include <doodle_lib/gui/widgets/derail/season_edit.h>
#include <doodle_lib/gui/widgets/derail/shot_edit.h>
#include <doodle_lib/gui/widgets/derail/time_edit.h>
#include <doodle_lib/gui/widgets/derail/user_edit.h>

#include <boost/signals2/connection.hpp>

#include "derail/importance_edit.h"
#include "derail/shot_edit.h"
#include <core/tree_node.h>

namespace doodle::gui {

class edit_widgets::impl {
 public:
  /**
   * @brief 修改句柄
   *
   */
  std::vector<entt::handle> p_h;

  render::season_edit_t season_edit{};
  render::episodes_edit_t episodes_edit{};
  render::shot_edit_t shot_edit{};
  render::assets_file_edit_t assets_file_edit{};
  render::command_edit_t command_edit{};
  render::importance_edit_t importance_edit{};
  render::time_edit_t time_edit{};

  std::string title_name_;
  bool open{true};
  boost::signals2::scoped_connection p_sc;
};

edit_widgets::edit_widgets() : p_i(std::make_unique<impl>()) {
  p_i->title_name_ = std::string{name};
  g_reg()->ctx().emplace<edit_widgets &>(*this);
  auto &l_sig = g_reg()->ctx().get<core_sig>();
  p_i->p_sc   = l_sig.select_handles.connect([&](const std::vector<entt::handle> &in) { p_i->p_h = in; });
}
edit_widgets::~edit_widgets() = default;

bool edit_widgets::render() {
  this->edit_handle();
  return p_i->open;
}

void edit_widgets::edit_handle() {
  /// @brief 资产编辑
  if (!p_i->p_h.empty()) {
    const auto l_args = p_i->p_h.size();
    if (l_args > 1) dear::Text(fmt::format("同时编辑了 {}个", l_args));
    p_i->season_edit.render(p_i->p_h.front());
    p_i->episodes_edit.render(p_i->p_h.front());
    p_i->shot_edit.render(p_i->p_h.front());
    p_i->assets_file_edit.render(p_i->p_h.front());
    p_i->command_edit.render(p_i->p_h.front());
    p_i->importance_edit.render(p_i->p_h.front());
    p_i->time_edit.render(p_i->p_h.front());
  }
}

const std::string &edit_widgets::title() const { return p_i->title_name_; }

}  // namespace doodle::gui

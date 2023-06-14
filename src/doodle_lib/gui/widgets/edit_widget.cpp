
//
// Created by TD on 2021/9/23.
//

#include "edit_widget.h"

#include "doodle_core/metadata/assets_file.h"
#include "doodle_core/metadata/comment.h"
#include "doodle_core/metadata/episodes.h"
#include "doodle_core/metadata/importance.h"
#include "doodle_core/metadata/season.h"
#include "doodle_core/metadata/shot.h"
#include "doodle_core/metadata/time_point_wrap.h"
#include <doodle_core/core/init_register.h>
#include <doodle_core/metadata/metadata.h>
#include <doodle_core/metadata/metadata_cpp.h>

#include <doodle_lib/gui/widgets/assets_file_widgets.h>
#include <doodle_lib/gui/widgets/derail/assets_file_edit.h>
#include <doodle_lib/gui/widgets/derail/command_edit.h>
#include <doodle_lib/gui/widgets/derail/episodes_edit.h>
#include <doodle_lib/gui/widgets/derail/importance_edit.h>
#include <doodle_lib/gui/widgets/derail/season_edit.h>
#include <doodle_lib/gui/widgets/derail/shot_edit.h>
#include <doodle_lib/gui/widgets/derail/time_edit.h>
#include <doodle_lib/gui/widgets/derail/user_edit.h>

#include <boost/signals2/connection.hpp>

#include "entt/entity/fwd.hpp"
#include "fmt/core.h"
#include "range/v3/algorithm/for_each.hpp"

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

  std::string database_info{};
};

edit_widgets::edit_widgets() : p_i(std::make_unique<impl>()) {
  p_i->title_name_ = std::string{name};
  g_reg()->ctx().emplace<edit_widgets &>(*this);
  auto &l_sig = g_reg()->ctx().get<core_sig>();
  p_i->p_sc   = l_sig.select_handles.connect([&](const std::vector<entt::handle> &in) {
    p_i->p_h = in;
    gen_text();
  });
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
    dear::Text(p_i->database_info);

    if (p_i->season_edit.render(p_i->p_h.front())) {
      auto l_data = p_i->p_h.front().get<season>();
      ranges::for_each(p_i->p_h, [&](const entt::handle &in) { in.patch<season>() = l_data; });
    }
    if (p_i->episodes_edit.render(p_i->p_h.front())) {
      auto l_data = p_i->p_h.front().get<episodes>();
      ranges::for_each(p_i->p_h, [&](const entt::handle &in) { in.patch<episodes>() = l_data; });
    }
    if (p_i->shot_edit.render(p_i->p_h.front())) {
      auto l_data = p_i->p_h.front().get<shot>();
      ranges::for_each(p_i->p_h, [&](const entt::handle &in) { in.patch<shot>() = l_data; });
    }
    if (p_i->assets_file_edit.render(p_i->p_h.front())) {
      auto l_data = p_i->p_h.front().get<assets_file>().user_attr();
      ranges::for_each(p_i->p_h, [&](const entt::handle &in) { in.patch<assets_file>().user_attr(l_data); });
    }
    if (p_i->command_edit.render(p_i->p_h.front())) {
      auto l_data = p_i->p_h.front().get<comment>();
      ranges::for_each(p_i->p_h, [&](const entt::handle &in) { in.patch<comment>() = l_data; });
    }
    if (p_i->importance_edit.render(p_i->p_h.front())) {
      auto l_data = p_i->p_h.front().get<importance>();
      ranges::for_each(p_i->p_h, [&](const entt::handle &in) { in.patch<importance>() = l_data; });
    }
    if (p_i->time_edit.render(p_i->p_h.front())) {
      auto l_data = p_i->p_h.front().get<time_point_wrap>();
      ranges::for_each(p_i->p_h, [&](const entt::handle &in) { in.patch<time_point_wrap>() = l_data; });
    }
  }
}

const std::string &edit_widgets::title() const { return p_i->title_name_; }
void edit_widgets::gen_text() {
  p_i->database_info.clear();
  for (auto i = 0; i < p_i->p_h.size() && i < 10; ++i) {
    p_i->database_info += fmt::format("id {} ", p_i->p_h[i].get<database>().get_id());
  }
  if (p_i->p_h.size() > 10) {
    p_i->database_info += "...";
  }
}

}  // namespace doodle::gui

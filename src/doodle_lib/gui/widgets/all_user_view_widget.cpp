//
// Created by TD on 2022/8/10.
//

#include "all_user_view_widget.h"

#include <doodle_core/metadata/rules.h>
#include <doodle_core/metadata/user.h>

#include <doodle_app/gui/base/ref_base.h>

#include <doodle_lib/gui/widgets/derail/user_edit.h>
#include <doodle_lib/gui/widgets/time_sequencer_widgets/time_rules_render.h>

#include <imgui.h>
#include <string>

namespace doodle::gui {
class all_user_view_widget::impl {
 public:
  struct user_gui_data {
    user_gui_data(const std::string& in_basic_string, entt::handle in_handle)
        : show_name(in_basic_string), handle(in_handle) {}

    gui_cache_name_id show_name;
    entt::handle handle;
  };

  std::string title_name_{std::string{name}};

  entt::handle select_user{};
  std::vector<user_gui_data> user_name_list{};

  business::rules rules_attr{};
  time_sequencer_widget_ns::time_rules_render time_rules_render_attr{};
  gui_cache_name_id delete_user{"删除用户"};

  /// 过滤用户
  render::select_all_user_t user_select{true};

  bool open{true};
  void get_user_time_rule() {
    if (select_user) {
      rules_attr = select_user.get_or_emplace<business::rules>();

      time_rules_render_attr.rules_attr(rules_attr);
    }
  };
  void rules_(const business::rules& in_rules) {
    if (!select_user) return;

    DOODLE_LOG_INFO("设置用户 {} 规则为 {}", select_user.get<user>(), in_rules);
    rules_attr = in_rules;
    select_user.replace<business::rules>(rules_attr);
  }
  void delete_user_fun(entt::handle& in_user) {
    in_user.destroy();

    boost::asio::post(g_io_context(), [=]() {
      user_name_list |=
          ranges::actions::remove_if([&](const user_gui_data& in) -> bool { return in.handle == in_user; });
    });
  }
};

all_user_view_widget::all_user_view_widget() : ptr(std::make_unique<impl>()) {}

bool all_user_view_widget::render() {
  if (auto&& [l_r, l_user] = ptr->user_select.render(g_reg()); l_r) {
    ptr->select_user = l_user;
    ptr->get_user_time_rule();
  }

  ImGui::SameLine();
  if (ImGui::Button(*ptr->delete_user)) {
    ptr->delete_user_fun(ptr->select_user);
  }

  if (ptr->time_rules_render_attr.render()) {
    ptr->rules_(ptr->time_rules_render_attr.rules_attr());
  }
  return ptr->open;
}
const std::string& all_user_view_widget::title() const { return ptr->title_name_; }

all_user_view_widget::~all_user_view_widget() = default;
}  // namespace doodle::gui

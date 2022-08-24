//
// Created by TD on 2022/8/10.
//

#include "all_user_view_widget.h"
#include <doodle_core/metadata/user.h>
#include <doodle_core/metadata/rules.h>

#include <doodle_lib/gui/gui_ref/ref_base.h>
#include <doodle_lib/gui/widgets/time_sequencer_widgets/time_rules_render.h>

namespace doodle::gui {
class all_user_view_widget::impl {
 public:
  struct user_gui_data {
    user_gui_data(const std::string& in_basic_string, entt::handle in_handle)
        : show_name(in_basic_string),
          handle(in_handle) {}

    gui_cache_name_id show_name;
    entt::handle handle;
  };

  entt::handle select_user{};
  std::vector<user_gui_data> user_name_list{};

  gui_cache_name_id delete_user{"删除所选用户"};
  gui_cache<std::string> combox_user_id{"查看用户"s, "null"s};
  business::rules rules_attr{};
  time_sequencer_widget_ns::time_rules_render time_rules_render_attr{};
  bool has_init{false};

  void get_all_user_data() {
    auto l_v = g_reg()->view<database, user>();
    for (auto&& [e, l_d, l_u] : l_v.each()) {
      user_name_list.emplace_back(fmt::to_string(l_u), make_handle(e));
    }
  };

  void get_user_time_rule(const entt::handle& in_h) {
    select_user = in_h;
    if (select_user) {
      rules_attr     = select_user.get_or_emplace<business::rules>();
      combox_user_id = fmt::to_string(in_h.get<user>());
      time_rules_render_attr.rules_attr(rules_attr);
    }
  };

  void rules_(const business::rules& in_rules) {
    DOODLE_LOG_INFO("设置用户 {} 规则为 {}", combox_user_id(), in_rules);
    rules_attr = in_rules;
    select_user.replace<business::rules>(rules_attr);
    database::save(select_user);
  }
  void delete_user_fun() {
    auto l_it = ranges::find_if(user_name_list, [&](const user_gui_data& in) -> bool {
      return in.handle == select_user;
    });
    if (l_it != user_name_list.end()) {
      database::delete_(l_it->handle);
      user_name_list.erase(l_it);
      combox_user_id() = std::string{"null"s};
    }
  }
};

all_user_view_widget::all_user_view_widget()
    : ptr(std::make_unique<impl>()) {
  title_name_ = std::string{name};
}
void all_user_view_widget::render() {
  if (!ptr->has_init) {
    ptr->get_all_user_data();
    ptr->has_init = true;
  }

  dear::Combo{*ptr->combox_user_id, ptr->combox_user_id().data()} && [this]() {
    for (auto&& l_u : ptr->user_name_list) {
      if (dear::Selectable(*l_u.show_name)) {
        ptr->get_user_time_rule(l_u.handle);
      }
    }
  };

  if (ImGui::Button(*ptr->delete_user)) {
    ptr->delete_user_fun();
  }

  if (ptr->time_rules_render_attr.render()) {
    ptr->rules_(ptr->time_rules_render_attr.rules_attr());
  }
}

all_user_view_widget::~all_user_view_widget() = default;
}  // namespace doodle::gui

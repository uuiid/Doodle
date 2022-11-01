//
// Created by TD on 2022/8/10.
//

#include "all_user_view_widget.h"

#include <doodle_core/metadata/rules.h>
#include <doodle_core/metadata/user.h>

#include <doodle_app/gui/base/ref_base.h>

#include <doodle_lib/gui/widgets/time_sequencer_widgets/time_rules_render.h>

namespace doodle::gui {
class all_user_view_widget::impl {
 public:
  struct user_gui_data {
    user_gui_data(const std::string& in_basic_string, const std::string& in_phone_number, entt::handle in_handle)
        : show_name(in_basic_string),
          phone_number(fmt::format("{}电话", in_basic_string), in_phone_number),
          handle(in_handle) {}

    gui_cache_name_id show_name;
    gui_cache_name_id get_chork;
    gui_cache<std::string> phone_number;
    gui_cache_name_id delete_user{"删除用户"};
    entt::handle handle;
  };

  std::string title_name_{std::string{name}};

  entt::handle select_user{};
  std::vector<user_gui_data> user_name_list{};

  gui_cache<std::string> combox_user_id{"查看用户"s, "null"s};
  business::rules rules_attr{};
  time_sequencer_widget_ns::time_rules_render time_rules_render_attr{};
  bool has_init{false};

  entt::observer del_user{};

  void get_all_user_data() {
    auto l_v = g_reg()->view<database, user>();
    for (auto&& [e, l_d, l_u] : l_v.each()) {
      auto l_h = make_handle(e);
      user_name_list.emplace_back(
          fmt::to_string(l_u), l_h.all_of<dingding::user>() ? l_h.get<dingding::user>().phone_number : ""s,
          make_handle(e)
      );
    }
    user_name_list |= ranges::action::sort([](const user_gui_data& in_l, const user_gui_data& in_r) {
      return in_l.show_name.name < in_r.show_name.name;
    });
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
  void delete_user_fun(const entt::handle& in_user) {
    database::delete_(in_user);

    boost::asio::post(g_io_context(), [=]() {
      user_name_list |=
          ranges::action::remove_if([&](const user_gui_data& in) -> bool { return in.handle == in_user; });
    });
  }
};

all_user_view_widget::all_user_view_widget() : ptr(std::make_unique<impl>()) {}

void all_user_view_widget::render() {
  if (!ptr->has_init) {
    ptr->get_all_user_data();
    ptr->has_init = true;
  }
  ImGui::PushItemWidth(260);
  for (auto& item : ptr->user_name_list) {
    if (dear::InputText(*item.phone_number, &item.phone_number)) {
      item.handle.get_or_emplace<dingding::user>().phone_number = item.phone_number;
      database::save(item.handle);
    }
    ImGui::SameLine();
    if (ImGui::Button(*item.delete_user)) {
      ptr->delete_user_fun(item.handle);
    }
  }
  ImGui::PopItemWidth();
  dear::Combo{*ptr->combox_user_id, ptr->combox_user_id().data()} && [this]() {
    for (auto&& l_u : ptr->user_name_list) {
      if (dear::Selectable(*l_u.show_name)) {
        ptr->get_user_time_rule(l_u.handle);
      }
    }
  };

  if (ptr->time_rules_render_attr.render()) {
    ptr->rules_(ptr->time_rules_render_attr.rules_attr());
  }
}
const std::string& all_user_view_widget::title() const { return ptr->title_name_; }

all_user_view_widget::~all_user_view_widget() = default;
}  // namespace doodle::gui

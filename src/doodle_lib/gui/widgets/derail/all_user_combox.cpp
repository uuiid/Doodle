#include "all_user_combox.h"

#include "doodle_core/logger/logger.h"
#include "doodle_core/metadata/metadata.h"
#include "doodle_core/metadata/user.h"

#include "doodle_app/gui/base/ref_base.h"
#include "doodle_app/lib_warp/imgui_warp.h"

#include <entt/entity/fwd.hpp>
#include <fmt/core.h>
#include <fmt/format.h>
#include <imgui.h>
#include <memory>
#include <string>

namespace doodle::gui {

namespace {
struct user_gui_data {
  explicit user_gui_data(const std::string& in_basic_string, entt::handle in_handle)
      : show_name(in_basic_string), handle(in_handle) {}

  gui_cache_name_id show_name;
  entt::handle handle;
};
}  // namespace
class all_user_combox::impl {
 public:
  bool show_delete{false};

  gui_cache<std::string> combox_user_id{"查看用户"s, "null"s};
  std::vector<user_gui_data> user_name_list{};

  entt::handle current_select_user;
  gui_cache_name_id delete_id{"删除用户"};

  std::string user_id;
  bool init{};
};

all_user_combox::all_user_combox() : ptr(std::make_unique<impl>()) {}

all_user_combox::all_user_combox(bool show_delete_button) : all_user_combox() { ptr->show_delete = show_delete_button; }

void all_user_combox::get_all_user_data() {
  auto l_v = g_reg()->view<database, user>();
  ptr->user_name_list.clear();
  for (auto&& [e, l_d, l_u] : l_v.each()) {
    auto l_h    = make_handle(e);
    auto l_name = fmt::to_string(l_u);
    if (l_name.empty()) l_name = fmt::to_string(l_d.uuid());
    ptr->user_name_list.emplace_back(l_name, make_handle(e));
  }
  ptr->user_name_list |= ranges::actions::sort([](const user_gui_data& in_l, const user_gui_data& in_r) {
    return in_l.show_name.name < in_r.show_name.name;
  });
}

void all_user_combox::delete_user(entt::handle& in_user) {
  if (in_user && in_user.all_of<database, user>()) {
    DOODLE_LOG_INFO("删除用户 {}", in_user.get<user>().get_name(), in_user.get<database>().uuid());
    in_user.destroy();
  } else {
    DOODLE_LOG_WARN("具有无效的句柄或者缺失组件");
  }
}

bool all_user_combox::render() {
  if (!ptr->init) {
    get_all_user_data();
    ptr->init = true;
  }

  bool l_r{};
  dear::Combo{*ptr->combox_user_id, ptr->combox_user_id().data()} && [this, l_r = &l_r]() {
    for (auto&& l_u : ptr->user_name_list) {
      if (dear::Selectable(*l_u.show_name)) {
        ptr->combox_user_id()    = l_u.show_name.name;
        ptr->current_select_user = l_u.handle;
        ptr->user_id             = fmt::to_string(l_u.handle.get<database>().uuid());
        *l_r                     = true;
      }
    }
  };

  if (ptr->show_delete) {
    ImGui::SameLine();
    if (ImGui::Button(*ptr->delete_id)) {
      delete_user(ptr->current_select_user);
    }
  }

  if (!ptr->user_id.empty()) {
    ImGui::SameLine();
    dear::Text(ptr->user_id);
  }

  return l_r;
}

entt::handle all_user_combox::get_user() { return ptr->current_select_user; }

all_user_combox::~all_user_combox() = default;

}  // namespace doodle::gui
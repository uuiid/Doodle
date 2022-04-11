//
// Created by TD on 2022/2/7.
//

#include "project_edit.h"
#include <metadata/project.h>
#include <doodle_lib/lib_warp/imgui_warp.h>
#include <doodle_lib/gui/gui_ref/project.h>
#include <doodle_lib/gui/gui_ref/database_edit.h>
#include <core/core_sig.h>
#include <doodle_lib/core/init_register.h>
namespace doodle {
namespace {
constexpr auto init = []() {
  entt::meta<project_edit>()
      .type()
      .prop("name"_hs, std::string{project_edit::name})
      .base<gui::window_panel>();
};
class init_class
    : public init_register::registrar_lambda<init, 3> {};
}

class project_edit::impl {
 public:
  using cache = gui::gui_cache<std::unique_ptr<gui::edit_interface>>;

  std::vector<cache> p_edits;
  gui::project_edit p_edit;
  gui::database_edit data_edit;
  entt::handle p_h;
};

project_edit::project_edit()
    : p_i(std::make_unique<impl>()) {
  title_name_ = std::string{name};
  p_i->p_edits.emplace_back("项目编辑", std::make_unique<gui::project_edit>());
  // p_i->p_edits.emplace_back("模型配置", std::make_unique<gui::modle_config_edit>());
  p_i->p_edits.emplace_back("解算配置", std::make_unique<gui::base_config_edit>());

  ranges::for_each(p_i->p_edits, [this](impl::cache& in_edit) {
    p_i->data_edit.link_sig(in_edit.data);
  });
}
project_edit::~project_edit() = default;

void project_edit::init() {
  p_i->p_h = project::get_current();
  p_i->data_edit.init(p_i->p_h);
  ranges::for_each(p_i->p_edits, [this](impl::cache& in) {
    in.data->init(p_i->p_h);
  });
}

void project_edit::failed() {
}

void project_edit::render() {
  p_i->data_edit.render(p_i->p_h);
  ImGui::Separator();
  ranges::for_each(p_i->p_edits, [this](impl::cache& in) {
    dear::Text(in.gui_name.name);
    in.data->render(p_i->p_h);
    in.data->save(p_i->p_h);
  });
  p_i->data_edit.save(p_i->p_h);
  if (ImGui::Button("保存"))
    g_reg()->ctx<core_sig>().save();
}
}  // namespace doodle

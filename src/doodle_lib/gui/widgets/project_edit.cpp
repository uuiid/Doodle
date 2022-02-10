//
// Created by TD on 2022/2/7.
//

#include "project_edit.h"
#include <metadata/project.h>
#include <doodle_lib/lib_warp/imgui_warp.h>
#include <doodle_lib/gui/gui_ref/project.h>

namespace doodle {

class project_edit::impl {
 public:
  gui::project_edit p_edit;
  entt::handle p_h;
};

project_edit::project_edit()
    : p_i(std::make_unique<impl>()) {
}
project_edit::~project_edit() = default;

void project_edit::init() {
  p_i->p_h = project::get_current();

  p_i->p_edit.init(p_i->p_h);
}
void project_edit::succeeded() {
}
void project_edit::failed() {
}
void project_edit::aborted() {
}
void project_edit::update(const chrono::duration<chrono::system_clock::rep,
                                                 chrono::system_clock::period>&,
                          void* data) {
  p_i->p_edit.render(p_i->p_h);
  p_i->p_edit.save(p_i->p_h);
}
}  // namespace doodle

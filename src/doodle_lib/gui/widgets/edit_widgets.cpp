
//
// Created by TD on 2021/9/23.
//

#include "edit_widgets.h"

#include <doodle_lib/gui/action/command.h>
#include <doodle_lib/gui/action/command_files.h>
#include <doodle_lib/gui/action/command_meta.h>
#include <doodle_lib/gui/action/command_ue4.h>
#include <doodle_lib/gui/action/command_video.h>
#include <doodle_lib/gui/factory/attribute_factory_interface.h>
#include <doodle_lib/metadata/metadata.h>

namespace doodle {
edit_widgets::edit_widgets() {
}

void edit_widgets::init() {
  g_reg()->set<edit_widgets &>(*this);
}
void edit_widgets::succeeded() {
}
void edit_widgets::failed() {
}
void edit_widgets::aborted() {
}
void edit_widgets::update(chrono::duration<chrono::system_clock::rep, chrono::system_clock::period>, void *data) {
 
}

}  // namespace doodle

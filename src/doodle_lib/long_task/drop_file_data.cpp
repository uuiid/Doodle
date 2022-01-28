//
// Created by TD on 2022/1/28.
//

#include "drop_file_data.h"
#include <doodle_lib/lib_warp/imgui_warp.h>
#include <long_task/process_pool.h>

namespace doodle {

drop_file_data::drop_file_data()
    : files_(),
      has_files() {
}
drop_file_data::~drop_file_data() = default;

void drop_file_data::drag_leave() {
  this->fail();
}
void drop_file_data::set_files(const std::vector<FSys::path> &in_paths) {
  files_    = in_paths;
  has_files = true;
  g_main_loop().attach<one_process_t>([this]() {
    this->succeed();
  });
}
void drop_file_data::init() {
  g_reg()->set<drop_file_data &>(*this);
}
void drop_file_data::succeeded() {
}
void drop_file_data::failed() {
}
void drop_file_data::aborted() {
}
void drop_file_data::update(const std::chrono::duration<std::chrono::system_clock::rep, std::chrono::system_clock::period> &, void *data) {
  dear::IDScope{this} && [this]() {
    dear::DragDropSource{
        ImGuiDragDropFlags_SourceExtern} &&
        [this]() {
          if (has_files) {
            ImGui::SetDragDropPayload(doodle_config::drop_imgui_id.data(),
                                      this, sizeof(*this));
          }
          ImGui::Text("开始拖拽文件");
        };
  };
}

}  // namespace doodle

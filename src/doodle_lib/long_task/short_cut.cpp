//
// Created by TD on 2022/2/10.
//

#include "short_cut.h"
#include <imgui.h>
#include <doodle_core/core/core_sig.h>
#include <doodle_core/core/init_register.h>
#include <doodle_core/thread_pool/process_pool.h>

namespace doodle {
class short_cut::impl {
 public:
  std::int32_t p_{0};
};

short_cut::short_cut() : p_i(std::make_unique<impl>()){};
void short_cut::init() {
  g_reg()->ctx().emplace<short_cut &>(*this);
}

void short_cut::update(const std::chrono::duration<std::chrono::system_clock::rep,
                                                   std::chrono::system_clock::period> &,
                       void *data) {
  if (ImGui::IsKeyPressed(ImGuiKey_S) && ImGui::GetIO().KeyCtrl)
    g_reg()->ctx().at<core_sig>().save();
}
short_cut::~short_cut() = default;

}  // namespace doodle

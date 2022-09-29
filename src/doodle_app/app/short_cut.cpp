//
// Created by TD on 2022/9/29.
//

#include "short_cut.h"
#include <imgui.h>
#include <doodle_core/core/core_sig.h>
#include <doodle_core/core/init_register.h>
#include <doodle_core/thread_pool/process_pool.h>

namespace doodle::gui {
class short_cut::impl {
 public:
  std::int32_t p_{0};
};

short_cut::short_cut() : p_i(std::make_unique<impl>()){};

bool short_cut::tick() {
  boost::ignore_unused(p_i);
  if (ImGui::IsKeyPressed(ImGuiKey_S) && ImGui::GetIO().KeyCtrl)
    g_reg()->ctx().at<core_sig>().save();
  return {};
}
short_cut::~short_cut() = default;

}  // namespace doodle::gui


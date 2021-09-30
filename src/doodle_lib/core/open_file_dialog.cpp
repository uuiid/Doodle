//
// Created by TD on 2021/9/22.
//

#include "open_file_dialog.h"

#include <doodle_lib/core/doodle_lib.h>
#include <doodle_lib/doodle_app.h>
namespace doodle {

void open_file_dialog::show(const std::function<void(const std::vector<FSys::path>&)>& in_fun) {
  doodle_app::Get()->main_loop.connect_extended(
      [kes = this->p_vKey, fun = in_fun](const doodle_app::connection& in) {
        dear::OpenFileDialog{kes} && [k_fun = fun, &in]() {
          auto ig = ImGuiFileDialog::Instance();
          if (ig->IsOk()) {
            auto filter           = ig->GetCurrentFilter();
            FSys::path k_get_curr_p = ig->GetFilePathName();
            auto k_get_curr            = k_get_curr_p.parent_path();
            std::vector<FSys::path> k_list{};
            auto selected = ig->GetSelection();
            std::transform(selected.begin(), selected.end(),
                           std::back_inserter(k_list),
                           [&k_get_curr](const auto& k) {
                             return k_get_curr / k.first;
                           });
            if (selected.empty())
              k_list.emplace_back(k_get_curr);
            k_fun(k_list);
          }
          in.disconnect();
        };
      });
};
}  // namespace doodle

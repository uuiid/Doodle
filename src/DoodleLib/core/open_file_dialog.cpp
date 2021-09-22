//
// Created by TD on 2021/9/22.
//

#include "open_file_dialog.h"

#include <DoodleLib/core/DoodleLib.h>
#include <DoodleLib/doodle_app.h>
namespace doodle {

void open_file_dialog::show(const std::function<void(const FSys::path&)>& in_fun) {
  doodle_app::Get()->main_loop.connect_extended(
      [this, &in_fun](const doodle_app::connection& in) {
        dear::OpenFileDialog{p_vKey} && [&in_fun, in]() {
          auto ig = ImGuiFileDialog::Instance();
          if (ig->IsOk()) {
            auto filter = ig->GetCurrentFilter();
            if (filter.empty()) {  ///选中文件夹
              FSys::path k_get_curr = ig->GetFilePathName();
              k_get_curr            = k_get_curr.parent_path();
              for (const auto& k : ig->GetSelection()) {
                in_fun(k_get_curr / k.first);
              }

            } else {  ///选中文件
              FSys::path k_get_curr = ig->GetFilePathName();
              for (const auto& k : ig->GetSelection()) {
                in_fun(k_get_curr / k.first);
              }
            }
          }
          in.disconnect();
        };
      });
};
}  // namespace doodle

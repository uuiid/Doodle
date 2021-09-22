//
// Created by TD on 2021/9/22.
//

#pragma once
#include <DoodleLib/DoodleLib_fwd.h>
#include <DoodleLib/libWarp/imgui_warp.h>

#include <utility>


namespace doodle {
class DOODLELIB_API open_file_dialog {
  std::string p_vKey;

 public:
  template <class... Args>
  open_file_dialog(
      std::string  vKey,
      Args&&... in_args)
      : p_vKey(std::move(vKey)) {
    ImGuiFileDialog::Instance()->OpenModal(
        p_vKey.c_str(),
        std::forward<Args>(in_args)...);
  };

  void show(const std::function<void(const FSys::path&)>& in_fun) ;
};
}  // namespace doodle

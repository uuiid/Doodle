//
// Created by TD on 2021/6/24.
//

#pragma once

#include <DoodleLib/DoodleLib_fwd.h>

#include <nana/gui.hpp>
#include <nana/gui/widgets/button.hpp>
#include <nana/gui/widgets/combox.hpp>
#include <nana/gui/widgets/form.hpp>
#include <nana/gui/widgets/label.hpp>
#include <nana/gui/widgets/spinbox.hpp>
#include <nana/gui/widgets/textbox.hpp>
#include <nana/gui/widgets/widget.hpp>
namespace doodle {
class DOODLELIB_API setting_windwos : public nana::form {
  nana::place p_layout;
  nana::combox p_dep;
  nana::textbox p_user;
  nana::label p_cache;
  nana::label p_doc;
  nana::textbox p_maya;
  nana::textbox p_ue_path;
  nana::textbox p_ue_version;
  nana::spinbox p_ue_shot_start;
  nana::spinbox p_ue_shot_end;

  void init_setting();
 public:
  explicit setting_windwos(nana::window in_window);
};
}  // namespace doodle

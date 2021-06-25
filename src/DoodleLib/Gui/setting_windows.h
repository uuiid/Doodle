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
class DOODLELIB_API setting_windows : public nana::form {
  nana::place p_layout;
  nana::label p_dep_label;
  nana::label p_user_label;
  nana::label p_cache_label;
  nana::label p_doc_label;
  nana::label p_maya_label;
  nana::label p_ue_path_label;
  nana::label p_ue_version_label;
  nana::label p_ue_shot_start_label;
  nana::label p_ue_shot_end_label;

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
  explicit setting_windows(nana::window in_window);
};
}  // namespace doodle

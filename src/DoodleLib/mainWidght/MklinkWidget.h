//
// Created by TD on 2021/5/10.
//

#pragma once

#include <DoodleLib/DoodleLib_fwd.h>
namespace doodle {
class MklinkWidget : public wxDialog {
  static FSys::path getFilePath(wxWindow* parent);

  FSys::path p_source;
  FSys::path p_target;

  bool CreateLink();

 public:
  explicit MklinkWidget(wxWindow* parent, wxWindowID id = wxID_ANY);
  static bool mklink(wxWindow* parent);
};

}  // namespace doodle

//
// Created by TD on 2021/5/10.
//

#pragma once

#include <DoodleLib/DoodleLib_fwd.h>
namespace doodle {
class MklinkWidget {

  FSys::path p_source;
  FSys::path p_target;


 public:
//  explicit MklinkWidget(wxWindow* parent, wxWindowID id = wxID_ANY);
  static bool mklink(const FSys::path& in_source, const FSys::path& in_target);
};

}  // namespace doodle

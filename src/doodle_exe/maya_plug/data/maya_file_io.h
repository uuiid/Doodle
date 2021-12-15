//
// Created by TD on 2021/12/6.
//

#pragma once

#include <doodle_lib/doodle_lib_fwd.h>
namespace doodle::maya_plug {

class maya_file_io {
 private:
 public:
  static FSys::path get_current_path();

  static FSys::path work_path(const FSys::path& in_path = ".");
  static string get_channel_date();
  static bool chick_channel();
  static bool replace_channel_date(const string& in_string);
  static bool save_file(const FSys::path& in_file_path);
};

}  // namespace doodle::maya_plug

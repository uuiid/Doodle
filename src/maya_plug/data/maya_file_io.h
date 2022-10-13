//
// Created by TD on 2021/12/6.
//

#pragma once

#include <doodle_core/doodle_core.h>

namespace doodle::maya_plug {
class reference_file;
class maya_file_io {
 private:
 public:
  static FSys::path get_current_path();

  static FSys::path work_path(const FSys::path& in_path = ".");
  static std::string get_channel_date();
  static bool chick_channel();
  static bool replace_channel_date(const std::string& in_string);
  static bool save_file(const FSys::path& in_file_path);

  static bool upload_file(const FSys::path& in_source_path, const FSys::path& in_prefix);

  static void import_reference_file(const reference_file& in_path, bool preserve_references);
};
}  // namespace doodle::maya_plug

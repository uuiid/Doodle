//
// Created by TD on 2021/12/6.
//

#pragma once

#include <doodle_core/doodle_core.h>

#include <maya_plug/main/maya_plug_fwd.h>

#include <maya/MFileIO.h>
namespace doodle::maya_plug {
class reference_file;
class maya_file_io {
 private:
 public:
  static FSys::path get_current_path();

  static FSys::path work_path(const FSys::path& in_path = ".");
  static void set_workspace(const FSys::path& in_path);
  static bool save_file(const FSys::path& in_file_path);
  static void open_file(
      const FSys::path& in_file_path, MFileIO::ReferenceMode in_mode = MFileIO::ReferenceMode::kLoadDefault
  );

};
}  // namespace doodle::maya_plug

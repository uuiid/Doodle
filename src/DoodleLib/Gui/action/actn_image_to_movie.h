//
// Created by TD on 2021/7/6.
//

#pragma once
#include <Doodlelib/DoodleLib_fwd.h>
#include <Doodlelib/Gui/action/action.h>

namespace doodle {
class DOODLELIB_API actn_image_to_movie : public action {
 public:
  class DOODLELIB_API arg_path : public _arg {
   public:
    arg_path() = default;
    arg_path(std::vector<FSys::path>& in_paths, FSys::path& in_out_path)
        : image_list(in_paths),
          out_file(in_out_path){};
    std::vector<FSys::path> image_list;
    FSys::path out_file;
  };

  using arg_ = arg_path;

  actn_image_to_movie() = default;

  bool is_accept(const _arg& in_any) override;
  void run(const MetadataPtr& in_data) override;
};

}  // namespace doodle

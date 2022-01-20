//
// Created by TD on 2022/1/20.
//

#pragma once
#include <doodle_lib/doodle_lib_fwd.h>

namespace doodle::gui{
template <>
struct adl_render<FSys::path> : adl_render_data<string> {
  bool render(FSys::path& in_path);
};
}

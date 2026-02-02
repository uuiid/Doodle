#pragma once

#include <maya_plug/data/reference_file.h>

#include <memory>
namespace doodle::maya_plug {

namespace reference_file_ns {
class generate_file_path_base;
}

using generate_file_path_ptr = std::shared_ptr<reference_file_ns::generate_file_path_base>;
class file_info_edit;

#define DOODLE_CHECK_MSTATUS_AND_RETURN_IT(in_status) \
  {                                                   \
    auto check_l_status = in_status;                        \
    CHECK_MSTATUS_AND_RETURN(check_l_status, check_l_status)      \
  }

}  // namespace doodle::maya_plug
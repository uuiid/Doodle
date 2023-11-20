//
// Created by TD on 2023/11/17.
//

#include "maya_to_exe_file.h"

#include <doodle_core/lib_warp/boost_fmt_error.h>
#include <doodle_core/logger/logger.h>
namespace doodle {

namespace maya_to_exe_file_ns {
struct maya_out_arg {
  FSys::path out_file{};
  // 引用文件
  FSys::path ref_file{};
  friend void from_json(const nlohmann::json &nlohmann_json_j, maya_out_arg &nlohmann_json_t) {
    nlohmann_json_j["out_file"].get_to(nlohmann_json_t.out_file);
    nlohmann_json_j["ref_file"].get_to(nlohmann_json_t.ref_file);
  };
};
}  // namespace maya_to_exe_file_ns

void maya_to_exe_file::operator()(boost::system::error_code in_error_code) const {
  if (in_error_code) {
    log_error(fmt::format("maya_to_exe_file error:{}", in_error_code));
    return;
  }

  auto l_maya_out_arg = nlohmann::json ::parse(maya_out_data_).get<std::vector<maya_to_exe_file_ns::maya_out_arg>>();
}
}  // namespace doodle
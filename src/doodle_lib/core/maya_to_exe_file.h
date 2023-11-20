//
// Created by TD on 2023/11/17.
//

#pragma once
#include <doodle_core/doodle_core_fwd.h>
namespace doodle {

class maya_to_exe_file {
 private:
  // maya 输出结果文件内容
  std::string maya_out_data_{};
  entt::handle msg_{};

 public:
  explicit maya_to_exe_file(std::string in_maya_out_data) : maya_out_data_(std::move(in_maya_out_data)) {
    msg_ = {*g_reg(), g_reg()->create()};
  };
  virtual ~maya_to_exe_file() = default;

  void operator()(boost::system::error_code in_error_code) const;
};

}  // namespace doodle

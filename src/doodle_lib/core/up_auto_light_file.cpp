//
// Created by TD on 2024/1/9.
//

#include "up_auto_light_file.h"

#include <doodle_core/metadata/project.h>

namespace doodle {
void up_auto_light_anim_file::init() { data_->logger_ = msg_.get<process_message>().logger(); }
void up_auto_light_anim_file::operator()(boost::system::error_code in_error_code, FSys::path in_gen_path) const {
  if (in_error_code) {
    data_->logger_->error("上传自动灯光文件失败:{}", in_error_code.message());
    wait_op_->ec_ = in_error_code;
    wait_op_->complete();
    return;
  }
  data_->logger_->info("开始上传文件夹 :{}", in_gen_path.generic_string());

  auto l_rem_path = msg_.get<project>().
}
}  // namespace doodle
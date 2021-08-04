//
// Created by TD on 2021/8/4.
//

#include "actn_tool_box.h"

#include <threadPool/long_term.h>

namespace doodle::toolbox {

actn_export_maya::actn_export_maya() {
  p_term = std::make_shared<long_term>();
  p_name = "导出maya fbx 文件";
}
long_term_ptr actn_export_maya::run() {
  p_date = sig_get_arg().value();

  if (p_date.is_cancel) {
    cancel("取消导出");
    return p_term;
  }
  return p_term;
}
bool actn_export_maya::is_async() {
  return true;
}

}  // namespace doodle::toolbox

#pragma once

#include <doodle_lib/core/app_base.h>

namespace doodle {
class DOODLELIB_API kitsu_supplement_main : public app_base {
 protected:
  using app_base::app_base;
  bool init() override;
  /// 检查是否有同名进程在监听 port 端口，若有则通过 stop-server 关闭旧实例
  void stop_previous_instance(std::uint16_t port, const std::string& in_secret);
};
}  // namespace doodle
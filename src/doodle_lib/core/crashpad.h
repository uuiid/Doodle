
#pragma once

#include <doodle_lib/doodle_lib_fwd.h>

#include <boost/core/noncopyable.hpp>


namespace doodle {
class crashpad_init : boost::noncopyable {
  crashpad_init();
  ~crashpad_init();

 public:
  // 获取单例
  static crashpad_init& get();
};
}  // namespace doodle
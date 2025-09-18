#pragma once
#include "doodle_core/core/file_sys.h"

#include <doodle_lib/doodle_lib_fwd.h>

namespace doodle {
class async_task {
 protected:
  logger_ptr logger_ptr_;

 public:
  virtual ~async_task()                      = default;
  virtual boost::asio::awaitable<void> run() = 0;

  // get set logger
  void set_logger(logger_ptr in_logger) { logger_ptr_ = in_logger; }
  logger_ptr get_logger() const { return logger_ptr_; }
};
}  // namespace doodle
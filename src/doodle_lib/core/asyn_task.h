#pragma once
#include <doodle_lib/doodle_lib_fwd.h>

namespace doodle {
class async_task {
 public:
  virtual ~async_task()                            = default;
  virtual boost::asio::awaitable<void> run() const = 0;
};
}  // namespace doodle
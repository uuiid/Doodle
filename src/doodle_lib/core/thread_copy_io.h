//
// Created by TD on 2024/1/6.
//

#pragma once
#include <doodle_core/doodle_core_fwd.h>

namespace doodle {

class thread_copy_io {
 public:
  thread_copy_io()  = default;
  ~thread_copy_io() = default;

  /**
   * @brief 这个是差异复制, 会自动比较文件的修改时间, 和大小 如果源文件的修改时间或者大小不同, 则会复制, 否则不会复制
   * @tparam CompletionHandler
   * @param from
   * @param to
   * @param in_options
   * @param handler
   * @return
   */
  template <typename CompletionHandler>
  auto copy(const FSys::path& from, const FSys::path& to, FSys::copy_options in_options, CompletionHandler&& handler) {}
};

}  // namespace doodle

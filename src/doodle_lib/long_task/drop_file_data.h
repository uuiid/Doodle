//
// Created by TD on 2022/1/28.
//

#pragma once
#include <doodle_lib/doodle_lib_fwd.h>

#include <utility>

namespace doodle {

class DOODLELIB_API drop_file_data : public process_t<drop_file_data> {
  bool has_files;

 public:
  std::vector<FSys::path> files_;
  drop_file_data();
  ~drop_file_data() override;

  void drag_leave();
  /**
   * @brief 设置拖拽上下文, 并在下一个循环析构, 本次循环是不可以析构的
   * @param in_paths 传入的路径
   */
  void set_files(const std::vector<FSys::path>& in_paths);

  [[maybe_unused]] void init();
  [[maybe_unused]] void succeeded();
  [[maybe_unused]] void failed();
  [[maybe_unused]] void aborted();
  [[maybe_unused]] void update(const delta_type&, void* data);
};

}  // namespace doodle

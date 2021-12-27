//
// Created by TD on 2021/12/27.
//

#pragma once
#include <doodle_lib/doodle_lib_fwd.h>
namespace doodle::details {
class DOODLELIB_API json_move : public process_t<json_move> {
 public:
  using base_type = process_t<json_move>;
  [[maybe_unused]] void init();
  [[maybe_unused]] void succeeded();
  [[maybe_unused]] void failed();
  [[maybe_unused]] void aborted();
  [[maybe_unused]] void update(base_type::delta_type, void *data);
};
}  // namespace doodle::details

//
// Created by TD on 2022/2/17.
//

#pragma once
#include <doodle_lib/doodle_lib_fwd.h>

namespace doodle {
namespace gui {

class DOODLELIB_API csv_export_widgets : public process_t<csv_export_widgets> {
  class impl;
  std::unique_ptr<impl> p_i;

 public:
  csv_export_widgets();
  ~csv_export_widgets();

  [[maybe_unused]] void init();
  [[maybe_unused]] void succeeded();
  [[maybe_unused]] void failed();
  [[maybe_unused]] void aborted();
  [[maybe_unused]] void update(const delta_type&, void* data);
};

}  // namespace gui
}  // namespace doodle

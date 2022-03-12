#pragma once

#include <doodle_lib/doodle_lib_fwd.h>

namespace doodle {
class DOODLELIB_API time_sequencer_widget
    : public process_t<time_sequencer_widget> {
 public:
  time_sequencer_widget();
  ~time_sequencer_widget() override;

  constexpr static std::string_view name{"时间序列"};

  [[maybe_unused]] void init();
  [[maybe_unused]] void succeeded();
  [[maybe_unused]] void failed();
  [[maybe_unused]] void aborted();
  [[maybe_unused]] void update(const delta_type&, void* data);
};
}  // namespace doodle

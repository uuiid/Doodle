//
// Created by TD on 2022/4/1.
//

#pragma once

#include <doodle_lib/doodle_lib_fwd.h>
namespace doodle {

class DOODLELIB_API ue4_widget : public process_t<ue4_widget> {
  class impl;
  std::unique_ptr<impl> p_i;


 public:
  ue4_widget();
  ~ue4_widget() override;
  constexpr static std::string_view name{"ue4工具"};

  [[maybe_unused]] void init();
  [[maybe_unused]] void succeeded();
  [[maybe_unused]] void failed();
  [[maybe_unused]] void aborted();
  [[maybe_unused]] void update(const delta_type&, void* data);
};

}  // namespace doodle

//
// Created by td_main on 2023/4/25.
//

#pragma once
#include <doodle_lib/exe_warp/maya_exe.h>

#include <functional>

namespace doodle::maya_plug {

class cloth_sim_factory {
 private:
  std::vector<std::function<void()>> run_args_{};

 public:
  cloth_sim_factory() = default;
  explicit cloth_sim_factory(const maya_exe_ns::qcloth_arg& in_arg) { preparation(in_arg); };
  virtual ~cloth_sim_factory() = default;

  void preparation(const maya_exe_ns::qcloth_arg& in_arg);

  void operator()() const;
};

}  // namespace doodle::maya_plug

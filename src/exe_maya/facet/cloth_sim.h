//
// Created by td_main on 2023/4/25.
//
#pragma once

#include <boost/program_options.hpp>

#include <string>

namespace doodle {
class cloth_sim final {
  std::string files_attr{};

  static constexpr char* config{"cloth_sim_config"};

 public:
  cloth_sim() = default;
  ~cloth_sim();

  const std::string& name() const noexcept;
  bool post();
  void deconstruction();
  void add_program_options();
};

}  // namespace doodle

//
// Created by TD on 2023/2/2.
//

#pragma once

#include <doodle_core/core/app_facet.h>

#include <boost/program_options.hpp>
namespace doodle::facet {

class create_move_facet {
  std::string name_{"create_move"};
  std::string files_attr;
  boost::program_options::options_description opt{"rpc"};

 public:
  create_move_facet() = default;
  [[nodiscard]] const std::string& name() const noexcept;
  void post();
  void deconstruction();
  void add_program_options();
};

}  // namespace doodle::facet

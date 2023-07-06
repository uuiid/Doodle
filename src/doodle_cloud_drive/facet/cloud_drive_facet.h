//
// Created by td_main on 2023/7/5.
//

#pragma once
namespace doodle {

class cloud_drive_arg {
 public:
  constexpr static const char* name = "cloud_drive_config";
};

class cloud_drive_facet {
 public:
  bool post();
  void add_program_options();
};

}  // namespace doodle

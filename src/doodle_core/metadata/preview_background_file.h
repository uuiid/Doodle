//
// Created by TD on 24-12-26.
//
#pragma once
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/base.h>
namespace doodle {
struct preview_background_file : base{
  std::string name_;
  bool archived_;
  bool is_default_;
  std::string original_name_;
  std::string extension_;
  std::int64_t file_size_;

};
}
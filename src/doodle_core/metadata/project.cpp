// Fill out your copyright notice in the Description page of Project Settings.
#include <doodle_core/metadata/project.h>

namespace doodle {

std::pair<std::int32_t, std::int32_t> project::get_resolution() const {
  if (resolution_.empty()) throw_exception(doodle_error{"无效的分辨率"});
  if (resolution_.find("x") == std::string::npos) throw_exception(doodle_error{"无效的分辨率"});
  return {
      std::stoi(resolution_.substr(0, resolution_.find("x"))), std::stoi(resolution_.substr(resolution_.find("x") + 1))
  };
}

}  // namespace doodle

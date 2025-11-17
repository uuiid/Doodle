// Fill out your copyright notice in the Description page of Project Settings.
#include <doodle_core/metadata/project.h>

#include <cmath>

namespace doodle {

std::pair<std::int32_t, std::int32_t> project::get_resolution() const {
  if (resolution_.empty()) throw_exception(doodle_error{"无效的分辨率"});
  if (resolution_.find("x") == std::string::npos) throw_exception(doodle_error{"无效的分辨率"});
  return {
      std::stoi(resolution_.substr(0, resolution_.find("x"))), std::stoi(resolution_.substr(resolution_.find("x") + 1))
  };
}

std::double_t project::get_film_aperture() const {
  std::double_t l_width{}, l_height{};
  if (ratio_.find(':') != std::string::npos) {
    l_width  = std::stod(ratio_.substr(0, ratio_.find(':')));
    l_height = std::stod(ratio_.substr(ratio_.find(':') + 1));
    if (l_height == 0.0) l_height = 1.0;
    return std::round((l_width / l_height) * 100) / 100;
  }
  return 1.78;
}

}  // namespace doodle

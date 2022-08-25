//
// Created by TD on 2021/7/29.
//

#include "season.h"
#include <doodle_core/logger/logger.h>

namespace doodle {
season::season()
    : p_int(0) {
}
season::season(std::int32_t in_)
    : p_int(in_) {
}

void season::set_season(std::int32_t in_) {
  p_int = in_;
}

std::int32_t season::get_season() const {
  return p_int;
}
std::string season::str() const {
  return fmt::format("seas_{}", p_int);
}

bool season::operator<(const season& in_rhs) const {
  return p_int < in_rhs.p_int;
}
bool season::operator>(const season& in_rhs) const {
  return in_rhs < *this;
}
bool season::operator<=(const season& in_rhs) const {
  return !(in_rhs < *this);
}
bool season::operator>=(const season& in_rhs) const {
  return !(*this < in_rhs);
}
bool season::operator==(const season& in_rhs) const {
  return p_int == in_rhs.p_int;
}
bool season::operator!=(const season& in_rhs) const {
  return !(in_rhs == *this);
}
bool season::analysis(const std::string& in_path) {
  static std::regex reg{R"(seas_?(\d+))", std::regex_constants::icase};
  std::smatch k_match{};
  const auto& k_r = std::regex_search(in_path, k_match, reg);
  if (k_r) {
    p_int = std::stoi(k_match[1].str());
  }
  return k_r;
}
bool season::analysis_static(const entt::handle& in_handle, const FSys::path& in_path) {
  season l_season{};
  if (l_season.analysis(in_path.generic_string())) {
    in_handle.emplace_or_replace<season>(l_season);
    return true;
  }
  return false;
}
}  // namespace doodle

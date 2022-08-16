#include "shot.h"

#include <doodle_core/metadata/episodes.h>
#include <doodle_core/logger/logger.h>
namespace doodle {

shot::shot()
    : shot(-1, shot_ab_enum::None) {
}
shot::shot(std::int64_t in_shot,
           shot_ab_enum in_ab)
    : p_shot(std::move(in_shot)),
      p_shot_enum(std::move(in_ab)),
      p_shot_ab(magic_enum::enum_name(p_shot_enum)) {
}
shot::shot(std::int64_t in_shot,
           std::string in_ab)
    : shot(in_shot, magic_enum::enum_cast<shot_ab_enum>(in_ab).value_or(shot_ab_enum::None)) {
}
const int64_t& shot::get_shot() const noexcept {
  return p_shot;
}

void shot::set_shot(const int64_t& in_shot) {
  DOODLE_CHICK(in_shot >= 0,doodle_error{"shot无法为负"});
  p_shot = in_shot;
}

std::string shot::get_shot_ab() const noexcept {
  return std::string{magic_enum::enum_name(p_shot_enum)};
}

shot::shot_ab_enum shot::get_shot_ab_enum() const noexcept {
  return p_shot_enum;
}

void shot::set_shot_ab(const std::string& ShotAb) noexcept {
  p_shot_enum = magic_enum::enum_cast<shot_ab_enum>(ShotAb)
                    .value_or(shot_ab_enum::None);
}

std::string shot::str() const {
  return fmt::format("sc{:04d}{}", p_shot,
                     p_shot_enum == shot_ab_enum::None ? "" : magic_enum::enum_name(p_shot_enum));
}
bool shot::operator<(const shot& rhs) const {
  return std::tie(p_shot, p_shot_enum) < std::tie(rhs.p_shot, rhs.p_shot_enum);
}
bool shot::operator>(const shot& rhs) const {
  return rhs < *this;
}
bool shot::operator<=(const shot& rhs) const {
  return !(rhs < *this);
}
bool shot::operator>=(const shot& rhs) const {
  return !(*this < rhs);
}

bool shot::analysis(const std::string& in_path) {
  static std::regex reg{R"(sc_?(\d+)([a-z])?)", std::regex_constants::icase};
  std::smatch k_match{};
  const auto& k_r = std::regex_search(in_path, k_match, reg);
  if (k_r) {
    p_shot = std::stoi(k_match[1].str());
    if (k_match.size() > 2)
      p_shot_enum = magic_enum::enum_cast<shot_ab_enum>(k_match[2].str())
                        .value_or(shot_ab_enum::None);
  }
  return k_r;
}

bool shot::analysis_static(const entt::handle& in_handle,
                           const FSys::path& in_path) {
  shot l_shot{};
  if (l_shot.analysis(in_path)) {
    in_handle.emplace_or_replace<shot>(l_shot);
    return true;
  }
  return false;
}

bool shot::operator==(const shot& in_rhs) const {
  return p_shot == in_rhs.p_shot &&
         p_shot_enum == in_rhs.p_shot_enum;
}
bool shot::operator!=(const shot& in_rhs) const {
  return !(in_rhs == *this);
}

}  // namespace doodle

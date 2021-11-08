#pragma once

#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/metadata/metadata.h>

#include <magic_enum.hpp>
namespace doodle {
class DOODLELIB_API shot {
 public:
  enum class shot_ab_enum;

 private:
  int64_t p_shot;
  std::string p_shot_ab;

 public:
  shot();
  shot(decltype(p_shot) in_shot,
       decltype(p_shot_ab) in_shot_ab = {});

  // clang-format off
  enum class shot_ab_enum { None, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z };
  // clang-format on

  [[nodiscard]] const std::int64_t &get_shot() const noexcept;
  void set_shot(const std::int64_t &in_shot);

  [[nodiscard]] const std::string &get_shot_ab() const noexcept;
  [[nodiscard]] shot_ab_enum get_shot_ab_enum() const noexcept;
  void set_shot_ab(const std::string &ShotAb) noexcept;
  inline void set_shot_ab(const shot_ab_enum &ShotAb) {
    set_shot_ab(std::string{magic_enum::enum_name(ShotAb)});
  };


  [[nodiscard]] std::string str() const;
  virtual void attribute_widget(const attribute_factory_ptr &in_factoryPtr);
  bool operator<(const shot &rhs) const;
  bool operator>(const shot &rhs) const;
  bool operator<=(const shot &rhs) const;
  bool operator>=(const shot &rhs) const;

  inline bool analysis(const FSys::path &in_path) {
    return analysis(in_path.generic_string());
  };
  bool analysis(const std::string &in_path);

  static std::optional<shot> analysis_static(const std::string &in_path);
  inline static std::optional<shot> analysis_static(const FSys::path &in_path) {
    return analysis_static(in_path.generic_string());
  };

 private:
  friend class boost::serialization::access;
  template <class Archive>
  void serialize(Archive &ar, std::uint32_t const version);
};
template <class Archive>
void shot::serialize(Archive &ar, const std::uint32_t version) {
  if (version == 1) {
    ar &BOOST_SERIALIZATION_NVP(p_shot);
    ar &BOOST_SERIALIZATION_NVP(p_shot_ab);
  }
}
}  // namespace doodle

namespace cereal {
template <class Archive>
std::string save_minimal(Archive const &, doodle::shot::shot_ab_enum const &shotab) {
  return std::string{magic_enum::enum_name(shotab)};
}
template <class Archive>
void load_minimal(Archive const &, doodle::shot::shot_ab_enum &shotab, std::string const &value) {
  shotab = magic_enum::enum_cast<doodle::shot::shot_ab_enum>(value).value_or(doodle::shot::shot_ab_enum::A);
};
}  // namespace cereal

BOOST_CLASS_VERSION(doodle::shot, 1)
BOOST_CLASS_EXPORT_KEY(doodle::shot)

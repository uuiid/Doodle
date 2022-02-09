#pragma once

#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/metadata/metadata.h>

#include <magic_enum.hpp>
namespace doodle {
class DOODLELIB_API shot {
 public:
  enum class shot_ab_enum;

  int64_t p_shot;
  shot_ab_enum p_shot_enum;

 public:
  std::string p_shot_ab;
  shot();
  explicit shot(std::int64_t in_shot,
                shot_ab_enum in_ab);
  explicit shot(std::int64_t in_shot,
                std::string in_ab);

  // clang-format off
  enum class shot_ab_enum { None, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z };
  // clang-format on

  [[nodiscard]] const std::int64_t &get_shot() const noexcept;
  void set_shot(const std::int64_t &in_shot);

  [[nodiscard]] std::string get_shot_ab() const noexcept;
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
  bool operator==(const shot &in_rhs) const;
  bool operator!=(const shot &in_rhs) const;
  inline bool analysis(const FSys::path &in_path) {
    return analysis(in_path.generic_string());
  };
  bool analysis(const std::string &in_path);

  static std::optional<shot> analysis_static(const std::string &in_path);
  inline static std::optional<shot> analysis_static(const FSys::path &in_path) {
    return analysis_static(in_path.generic_string());
  };

 private:
  friend void to_json(nlohmann::json &j, const shot &p) {
    j["shot"]      = p.p_shot;
    j["shot_enum"] = p.p_shot_enum;
  }
  friend void from_json(const nlohmann::json &j, shot &p) {
    j.at("shot").get_to(p.p_shot);
    j.at("shot_enum").get_to(p.p_shot_enum);
  }
};

}  // namespace doodle

namespace fmt {
template <>
struct fmt::formatter<::doodle::shot> : fmt::formatter<fmt::string_view> {
  template <typename FormatContext>
  auto format(const ::doodle::shot &in_, FormatContext &ctx) -> decltype(ctx.out()) {
    return formatter<string_view>::format(
        in_.str(),
        ctx);
  }
};
}  // namespace fmt

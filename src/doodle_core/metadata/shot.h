#pragma once

#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/metadata.h>

#include <magic_enum.hpp>

namespace doodle {
class DOODLE_CORE_API shot {
 public:
  enum class shot_ab_enum;

  int32_t p_shot;
  shot_ab_enum p_shot_enum;

 public:
  std::string p_shot_ab;
  shot();
  explicit shot(std::int32_t in_shot, shot_ab_enum in_ab);
  explicit shot(std::int32_t in_shot, std::string in_ab);

  // clang-format off
  enum class shot_ab_enum { None, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z };
  // clang-format on

  [[nodiscard]] const std::int32_t &get_shot() const noexcept;
  void set_shot(const std::int32_t &in_shot);

  [[nodiscard]] std::string get_shot_ab() const noexcept;
  [[nodiscard]] shot_ab_enum get_shot_ab_enum() const noexcept;
  void set_shot_ab(const std::string &ShotAb) noexcept;
  inline void set_shot_ab(const shot_ab_enum &ShotAb) { set_shot_ab(std::string{magic_enum::enum_name(ShotAb)}); };

  [[nodiscard]] std::string str() const;
  bool operator<(const shot &rhs) const;
  bool operator>(const shot &rhs) const;
  bool operator<=(const shot &rhs) const;
  bool operator>=(const shot &rhs) const;
  bool operator==(const shot &in_rhs) const;
  bool operator!=(const shot &in_rhs) const;
  inline bool analysis(const FSys::path &in_path) { return analysis(in_path.generic_string()); };
  bool analysis(const std::string &in_path);

  static bool analysis_static(const entt::handle &in_handle, const FSys::path &in_path);

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
/**
 * @brief 镜头格式化程序
 *
 * @tparam
 */
template <>
struct formatter<::doodle::shot> : formatter<std::int32_t> {
  template <typename FormatContext>
  auto format(const ::doodle::shot &in_, FormatContext &ctx) const -> decltype(ctx.out()) {
    format_to(ctx.out(), "sc_");

    formatter<std::int32_t>::format(in_.p_shot, ctx);
    if (in_.p_shot_enum != doodle::shot::shot_ab_enum::None)
      format_to(ctx.out(), magic_enum::enum_name(in_.p_shot_enum));
    return ctx.out();
  }
};

/**
 * @brief 镜头枚举格式化程序
 *
 * @tparam
 */
template <>
struct formatter<::doodle::shot::shot_ab_enum> : formatter<fmt::string_view> {
  template <typename FormatContext>
  auto format(const ::doodle::shot::shot_ab_enum &in_, FormatContext &ctx) const -> decltype(ctx.out()) {
    switch (in_) {
      case ::doodle::shot::shot_ab_enum::None:
        break;
      default:
        formatter<fmt::string_view>::format(magic_enum::enum_name(in_), ctx);
        break;
    }

    return ctx.out();
  }
};
}  // namespace fmt

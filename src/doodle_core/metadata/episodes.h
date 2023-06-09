#pragma once
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/metadata.h>

namespace doodle {

class DOODLE_CORE_API episodes {
 public:
  std::int32_t p_episodes;
  episodes();
  explicit episodes(std::int32_t in_episodes);

  [[nodiscard]] const std::int32_t &get_episodes() const noexcept;
  void set_episodes(const std::int32_t &Episodes_);

  bool operator<(const episodes &in_rhs) const;
  bool operator>(const episodes &in_rhs) const;
  bool operator<=(const episodes &in_rhs) const;
  bool operator>=(const episodes &in_rhs) const;
  bool operator==(const episodes &in_rhs) const;
  bool operator!=(const episodes &in_rhs) const;
  inline bool analysis(const FSys::path &in_path) { return analysis(in_path.generic_string()); };
  bool analysis(const std::string &in_path);

  static bool analysis_static(const entt::handle &in_handle, const FSys::path &in_path);

  static bool conjecture_season(const entt::handle &in_handle);

 private:
  friend void to_json(nlohmann::json &j, const episodes &p) { j["episodes"] = p.p_episodes; }
  friend void from_json(const nlohmann::json &j, episodes &p) { j.at("episodes").get_to(p.p_episodes); }
};

}  // namespace doodle

namespace fmt {
/**
 * @brief 集数格式化程序
 *
 * @tparam
 */
template <>
struct formatter<::doodle::episodes> : formatter<std::int32_t> {
  template <typename FormatContext>
  auto format(const ::doodle::episodes &in_, FormatContext &ctx) const -> decltype(ctx.out()) {
    format_to(ctx.out(), "ep_");
    return formatter<std::int32_t>::format(in_.p_episodes, ctx);
  }
};
}  // namespace fmt

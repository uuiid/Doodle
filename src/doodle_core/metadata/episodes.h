#pragma once
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/entity.h>
#include <doodle_core/metadata/metadata.h>


namespace doodle {

class DOODLE_CORE_API episodes {
 public:
  std::int32_t p_episodes;
  episodes();
  explicit episodes(std::int32_t in_episodes);
  explicit episodes(const entity& in_entity);

  [[nodiscard]] const std::int32_t& get_episodes() const noexcept;
  void set_episodes(const std::int32_t& Episodes_);

  bool operator<(const episodes& in_rhs) const;
  bool operator>(const episodes& in_rhs) const;
  bool operator<=(const episodes& in_rhs) const;
  bool operator>=(const episodes& in_rhs) const;
  bool operator==(const episodes& in_rhs) const;
  bool operator!=(const episodes& in_rhs) const;
  inline bool analysis(const FSys::path& in_path) { return analysis(in_path.generic_string()); };
  bool analysis(const std::string& in_path);

  friend std::size_t hash_value(episodes const& value) { return std::hash<std::int32_t>{}(value.p_episodes); }

 private:
  friend void to_json(nlohmann::json& j, const episodes& p) { j = p.p_episodes; }
  friend void from_json(const nlohmann::json& j, episodes& p) { j.get_to(p.p_episodes); }
};

}  // namespace doodle

namespace fmt {
/**
 * @brief 集数格式化程序
 *
 */
template <>
struct formatter<::doodle::episodes> : formatter<std::string> {
  template <typename FormatContext>
  auto format(const ::doodle::episodes& in_, FormatContext& ctx) const -> decltype(ctx.out()) {
    auto l_str = fmt::format("ep_{}", in_.p_episodes);
    return formatter<std::string>::format(l_str, ctx);
  }
};
}  // namespace fmt
namespace std {
template <>
struct hash<doodle::episodes> : hash<std::int32_t> {
  std::size_t operator()(const doodle::episodes& value) const noexcept {
    return hash<std::int32_t>::operator()(value.p_episodes);
  }
};
}  // namespace std
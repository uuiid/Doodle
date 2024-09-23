//
// Created by TD on 2021/7/29.
//

#pragma once
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/metadata.h>

namespace doodle {

class DOODLE_CORE_API season {
 public:
  std::int32_t p_int;
  season();
  explicit season(std::int32_t in_);

  void set_season(std::int32_t in_);
  std::int32_t get_season() const;

  virtual std::string str() const;
  bool operator<(const season& in_rhs) const;
  bool operator>(const season& in_rhs) const;
  bool operator<=(const season& in_rhs) const;
  bool operator>=(const season& in_rhs) const;
  bool operator==(const season& in_rhs) const;
  bool operator!=(const season& in_rhs) const;

  bool analysis(const std::string& in_path);

  static bool analysis_static(const entt::handle& in_handle, const FSys::path& in_path);
  friend std::size_t hash_value(season const& value) { return std::hash<std::int32_t>{}(value.p_int); }

 private:
  friend void to_json(nlohmann::json& j, const season& p) { j["season"] = p.p_int; }
  friend void from_json(const nlohmann::json& j, season& p) { j.at("season").get_to(p.p_int); }
};

}  // namespace doodle

namespace fmt {
/**
 * @brief 季数格式化程序
 *
 */
template <>
struct formatter<::doodle::season> : formatter<std::string> {
  template <typename FormatContext>
  auto format(const ::doodle::season& in_, FormatContext& ctx) const -> decltype(ctx.out()) {
    auto l_str = fmt::format("seas_{}", in_.p_int);
    return formatter<std::string>::format(l_str, ctx);
  }
};
}  // namespace fmt

namespace std {
template <>
struct hash<doodle::season> : hash<std::int32_t> {
  std::size_t operator()(const doodle::season& value) const noexcept {
    return hash<std::int32_t>::operator()(value.p_int);
  }
};
}
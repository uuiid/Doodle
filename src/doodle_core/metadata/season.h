//
// Created by TD on 2021/7/29.
//

#pragma once
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/metadata.h>
namespace doodle {

class DOODLE_CORE_EXPORT season {
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

  static bool analysis_static(const entt::handle& in_handle,
                              const FSys::path& in_path);

 private:
  friend void to_json(nlohmann::json& j, const season& p) {
    j["season"] = p.p_int;
  }
  friend void from_json(const nlohmann::json& j, season& p) {
    j.at("season").get_to(p.p_int);
  }
};

}  // namespace doodle

namespace fmt {
/**
 * @brief 季数格式化程序
 *
 * @tparam
 */
template <>
struct formatter<::doodle::season> : formatter<std::int32_t> {
  template <typename FormatContext>
  auto format(const ::doodle::season& in_, FormatContext& ctx) const -> decltype(ctx.out()) {
    format_to(ctx.out(), "seas_");
    return formatter<std::int32_t>::format(
        in_.p_int,
        ctx);
  }
};
}  // namespace fmt

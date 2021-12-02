#pragma once
#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/metadata/metadata.h>

namespace doodle {

class DOODLELIB_API episodes {
 public:
  int64_t p_episodes;
  episodes();
  explicit episodes(int64_t in_episodes);
  // ~Episodes();
  [[nodiscard]] const int64_t &get_episodes() const noexcept;
  void set_episodes(const int64_t &Episodes_);

  [[nodiscard]] std::string str() const;

  void attribute_widget(const attribute_factory_ptr &in_factoryPtr);

  bool operator<(const episodes &in_rhs) const;
  bool operator>(const episodes &in_rhs) const;
  bool operator<=(const episodes &in_rhs) const;
  bool operator>=(const episodes &in_rhs) const;
  bool operator==(const episodes &in_rhs) const;
  bool operator!=(const episodes &in_rhs) const;
  inline bool analysis(const FSys::path &in_path) {
    return analysis(in_path.generic_string());
  };
  bool analysis(const std::string &in_path);

  static std::optional<episodes> analysis_static(const std::string &in_path);
  inline static std::optional<episodes> analysis_static(const FSys::path &in_path) {
    return analysis_static(in_path.generic_string());
  };

 private:

  friend void to_json(nlohmann::json &j, const episodes &p) {
    j["episodes"] = p.p_episodes;
  }
  friend void from_json(const nlohmann::json &j, episodes &p) {
    j.at("episodes").get_to(p.p_episodes);
  }
};


}  // namespace doodle

namespace fmt {
template <>
struct fmt::formatter<::doodle::episodes> : fmt::formatter<fmt::string_view> {
  template <typename FormatContext>
  auto format(const ::doodle::episodes &in_, FormatContext &ctx) -> decltype(ctx.out()) {
    return formatter<string_view>::format(
        in_.str(),
        ctx);
  }
};
}  // namespace fmt


//
// Created by TD on 2021/5/18.
//

#pragma once
#include <doodle_core/doodle_core_fwd.h>

#include <rttr/rttr_enable.h>
namespace doodle {

class DOODLE_CORE_API comment {
  RTTR_ENABLE();

 public:
  std::string p_comment;
  std::string p_time_info;
  comment();
  explicit comment(std::string in_str);
  [[nodiscard]] const std::string& get_comment() const;
  void set_comment(const std::string& in_comment);

 private:
  friend void to_json(nlohmann::json& j, const comment& p) {
    j["comment"]   = p.p_comment;
    j["time_info"] = p.p_time_info;
  }
  friend void from_json(const nlohmann::json& j, comment& p) {
    j.at("comment").get_to(p.p_comment);
    if (j.contains("time_info")) j.at("time_info").get_to(p.p_time_info);
  }
};

}  // namespace doodle

namespace fmt {
template <>
struct formatter<doodle::comment> : formatter<string_view> {
  template <typename FormatContext>
  auto format(const doodle::comment& in_, FormatContext& ctx) const {
    formatter<string_view>::format(in_.get_comment(), ctx);
  }
};
}  // namespace fmt

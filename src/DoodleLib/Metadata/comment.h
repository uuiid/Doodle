//
// Created by TD on 2021/5/18.
//

#pragma once
#include <DoodleLib/DoodleLib_fwd.h>
namespace doodle {

class DOODLELIB_API comment {
  std::string p_comment;
  std::string p_user;

 public:
  comment();
  explicit comment(std::string in_str);
  [[nodiscard]] const std::string& get_comment() const;
  void set_comment(const std::string& in_comment);
  [[nodiscard]] const std::string& get_user() const;
  void set_user(const std::string& in_user);

 private:
  friend class boost::serialization::access;
  template <class Archive>
  void serialize(Archive& ar, std::uint32_t const version);
};
template <class Archive>
void comment::serialize(Archive& ar, const std::uint32_t version) {
  if (version == 1)
    ar&BOOST_SERIALIZATION_NVP(p_comment)&
       BOOST_SERIALIZATION_NVP(p_user);
}
}  // namespace doodle

namespace fmt {
template <>
struct formatter<doodle::comment> : formatter<string_view> {
  template <typename FormatContext>
  auto format(const doodle::comment& in_, FormatContext& ctx) {
    formatter<string_view>::format(
        in_.get_comment(),
        ctx);
  }
};
}  // namespace fmt
BOOST_CLASS_VERSION(doodle::comment, 1)
BOOST_CLASS_EXPORT_KEY(doodle::comment)

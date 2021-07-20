//
// Created by TD on 2021/5/18.
//

#pragma once
#include <DoodleLib/DoodleLib_fwd.h>
namespace doodle {

class DOODLELIB_API Comment {
  std::string p_comment;
  std::string p_user;

 public:
  Comment();
  explicit Comment(std::string in_str);
  [[nodiscard]] const std::string& getComment() const;
  void setComment(const std::string& in_comment);
  [[nodiscard]] const std::string& getUser() const;
  void setUser(const std::string& in_user);

 private:
  friend class cereal::access;
  template <class Archive>
  void serialize(Archive& ar, std::uint32_t const version);
};
template <class Archive>
void Comment::serialize(Archive& ar, const std::uint32_t version) {
  if (version == 1)
    ar(CEREAL_NVP(p_comment),
       CEREAL_NVP(p_user));
}
}  // namespace doodle

namespace fmt {
template <>
struct fmt::formatter<doodle::Comment> : formatter<string_view> {
  template <typename FormatContext>
  auto format(const doodle::Comment& in_, FormatContext& ctx) {
    formatter<string_view>::format(
        in_.getComment(),
        ctx);
  }
};
}  // namespace fmt
CEREAL_CLASS_VERSION(doodle::Comment, 1)

//
// Created by TD on 2021/5/18.
//

#include <DoodleLib/Metadata/comment.h>
#include <DoodleLib/core/core_set.h>

BOOST_CLASS_EXPORT_IMPLEMENT(doodle::comment)
namespace doodle {
comment::comment()
    : p_comment(),
      p_user(core_set::getSet().getUser()) {
}

comment::comment(std::string in_str)
    : p_comment(std::move(in_str)),
      p_user(core_set::getSet().getUser()) {
}

const std::string& comment::getComment() const {
  return p_comment;
}
void comment::setComment(const std::string& in_comment) {
  p_comment = in_comment;
}
const std::string& comment::getUser() const {
  return p_user;
}
void comment::setUser(const std::string& in_user) {
  p_user = in_user;
}
}  // namespace doodle

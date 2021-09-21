//
// Created by TD on 2021/5/18.
//

#include <DoodleLib/Metadata/Comment.h>
#include <DoodleLib/core/CoreSet.h>

BOOST_CLASS_EXPORT_IMPLEMENT(doodle::Comment)
namespace doodle {
Comment::Comment()
    : p_comment(),
      p_user(CoreSet::getSet().getUser()) {
}

Comment::Comment(std::string in_str)
    : p_comment(std::move(in_str)),
      p_user(CoreSet::getSet().getUser()) {
}

const std::string& Comment::getComment() const {
  return p_comment;
}
void Comment::setComment(const std::string& in_comment) {
  p_comment = in_comment;
}
const std::string& Comment::getUser() const {
  return p_user;
}
void Comment::setUser(const std::string& in_user) {
  p_user = in_user;
}
}  // namespace doodle

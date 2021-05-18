//
// Created by TD on 2021/5/18.
//

#include <DoodleLib/Metadata/Comment.h>
namespace doodle {
Comment::Comment()
    : p_comment(),
      p_user() {
}

Comment::Comment(std::string in_str)
    : p_comment(std::move(in_str)),
      p_user() {
}

const std::string& Comment::GetComment() const {
  return p_comment;
}
void Comment::SetComment(const std::string& in_comment) {
  p_comment = in_comment;
}
const std::string& Comment::GetUser() const {
  return p_user;
}
void Comment::SetUser(const std::string& in_user) {
  p_user = in_user;
}
}  // namespace doodle

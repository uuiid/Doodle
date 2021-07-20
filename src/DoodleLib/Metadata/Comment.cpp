﻿//
// Created by TD on 2021/5/18.
//

#include <DoodleLib/Metadata/Comment.h>
#include <DoodleLib/core/CoreSet.h>
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
std::string comment_list::str() const {
  std::string str{};
  for (auto& k_item : *this) {
    str += fmt::format("{}\n", *k_item);
  }
  return str;
}
}  // namespace doodle

//
// Created by TD on 2021/5/18.
//

#include <core/core_set.h>
#include <doodle_core/metadata/comment.h>
#include <doodle_core/metadata/metadata.h>

namespace doodle {
comment::comment()
    : comment(std::string{}) {
}

comment::comment(std::string in_str)
    : p_comment(std::move(in_str)) {
}

const std::string& comment::get_comment() const {
  return p_comment;
}
void comment::set_comment(const std::string& in_comment) {
  p_comment = in_comment;
}

}  // namespace doodle

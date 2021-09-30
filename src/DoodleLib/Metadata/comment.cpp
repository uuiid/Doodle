//
// Created by TD on 2021/5/18.
//

#include <DoodleLib/Metadata/comment.h>
#include <DoodleLib/Metadata/metadata.h>
#include <DoodleLib/core/core_set.h>

BOOST_CLASS_EXPORT_IMPLEMENT(doodle::comment)
BOOST_CLASS_EXPORT_IMPLEMENT(doodle::comment_vector)
namespace doodle {
comment::comment()
    : p_comment(),
      p_user(core_set::getSet().get_user()) {
}

comment::comment(std::string in_str)
    : p_comment(std::move(in_str)),
      p_user(core_set::getSet().get_user()) {
}

const std::string& comment::get_comment() const {
  return p_comment;
}
void comment::set_comment(const std::string& in_comment) {
  p_comment = in_comment;
  p_meta.lock()->saved(true);
}
const std::string& comment::get_user() const {
  return p_user;
}
void comment::set_user(const std::string& in_user) {
  p_user = in_user;
}

void comment_vector::set_metadata(const metadata_ptr& in_meta) {
  leaf_meta::set_metadata(in_meta);
  for (auto& i : get()) {
    i->set_metadata(in_meta);
  }
}

}  // namespace doodle

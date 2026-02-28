#pragma once

#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/base.h>
#include <doodle_core/metadata/comment.h>

namespace doodle::comment_ns {
void set_comment_mentions(const comment& in_comment, const uuid& in_project_id);
void set_comment_department_mentions(const comment& in_comment);
}  // namespace doodle::comment_ns
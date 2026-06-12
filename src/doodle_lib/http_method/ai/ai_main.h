//
// Created by TD on 25-5-13.
//
#pragma once
#include <doodle_lib/core/http/http_function.h>

namespace doodle::http {

// /api/doodle/ai/animation/train
DOODLE_HTTP_FUN(ai_train_animation)
struct impl;
std::shared_ptr<impl> impl_ptr_;
ai_train_animation();
DOODLE_HTTP_FUN_OVERRIDE(post)
DOODLE_HTTP_FUN_END()
}  // namespace doodle::http
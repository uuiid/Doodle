//
// Created by TD on 25-5-13.
//
#pragma once
#include <doodle_lib/core/http/http_function.h>

namespace doodle::http {

// /api/doodle/ai/train-binding-weights
DOODLE_HTTP_FUN(ai_train_binding_weights)
DOODLE_HTTP_FUN_OVERRIDE(post)
DOODLE_HTTP_FUN_OVERRIDE(put)
DOODLE_HTTP_FUN_END()
}  // namespace doodle::http
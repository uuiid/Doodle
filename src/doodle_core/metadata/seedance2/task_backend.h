#pragma once

#include <doodle_core/doodle_core_fwd.h>

#include <magic_enum/magic_enum.hpp>

namespace doodle::seedance2 {

enum class DOODLE_CORE_API task_backend {
  seedance2,
  transfer_station,
  apipy_image,
  gemini_image,
  kmt_image,
};
DOODLE_JSON_SERIALIZE_ENUM_STRICT(
    task_backend,
    {{task_backend::seedance2, "seedance2"},
     {task_backend::transfer_station, "transfer_station"},
     {task_backend::apipy_image, "apipy_image"},
     {task_backend::gemini_image, "gemini_image"},
     {task_backend::kmt_image, "kmt_image"}}
);

}  // namespace doodle::seedance2

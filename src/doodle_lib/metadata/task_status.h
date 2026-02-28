
#pragma once
#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_core/metadata/task.h>
#include <doodle_core/metadata/task_status.h>

namespace doodle::task_status_ns {
void check_retake_capping(const task_status& in_task_status, const task& in_task);
}
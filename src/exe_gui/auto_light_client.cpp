//
// Created by td_main on 2023/9/11.
//
#include <doodle_core/core/app_service.h>

#include <doodle_lib/launch/http_working_service.h>

#include "main_macro.h"
DOODLE_SERVICE_MAIN_IMPL(app_service_t, doodle::launch::http_working_service_t);
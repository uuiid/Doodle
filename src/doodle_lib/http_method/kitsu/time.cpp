//
// Created by TD on 25-4-3.
//
#include <doodle_lib/http_method/kitsu/kitsu_reg_url.h>

namespace doodle::http {
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(data_user_time_spents_all, get) {
  co_return in_handle->make_msg(nlohmann::json::array());
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(data_user_time_spents, get) {
  co_return in_handle->make_msg(nlohmann::json::array());
}
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(person_day_off, get) {
  co_return in_handle->make_msg(nlohmann::json::object());
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(person_day_off_all, get) {
  co_return in_handle->make_msg(nlohmann::json::array());
}
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(person_time_spents_day_table, get) {
  co_return in_handle->make_msg(nlohmann::json::array());
}
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(person_day_off_1, get) {
  co_return in_handle->make_msg(nlohmann::json::array());
}

}  // namespace doodle::http
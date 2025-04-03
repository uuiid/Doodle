//
// Created by TD on 25-4-3.
//
#include <doodle_lib/http_method/kitsu/kitsu_reg_url.h>

namespace doodle::http {
boost::asio::awaitable<boost::beast::http::message_generator> data_user_time_spents_all_get::callback(
    session_data_ptr in_handle
) {
  co_return in_handle->make_msg("[]");
}

boost::asio::awaitable<boost::beast::http::message_generator> data_user_time_spents_get::callback(
    session_data_ptr in_handle
) {
  co_return in_handle->make_msg("[]");
}
boost::asio::awaitable<boost::beast::http::message_generator> person_day_off_get::callback(session_data_ptr in_handle) {
  co_return in_handle->make_msg("{}");
}

boost::asio::awaitable<boost::beast::http::message_generator> person_day_off_all_get::callback(
    session_data_ptr in_handle
) {
  co_return in_handle->make_msg("[]");
}

}  // namespace doodle::http
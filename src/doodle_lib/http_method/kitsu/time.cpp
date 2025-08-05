//
// Created by TD on 25-4-3.
//
#include <doodle_lib/http_method/kitsu/kitsu_reg_url.h>

namespace doodle::http {
boost::asio::awaitable<boost::beast::http::message_generator> data_user_time_spents_all::get(
    session_data_ptr in_handle
) {
  co_return in_handle->make_msg(nlohmann::json::array());
}

boost::asio::awaitable<boost::beast::http::message_generator> data_user_time_spents::get(session_data_ptr in_handle) {
  co_return in_handle->make_msg(nlohmann::json::array());
}
boost::asio::awaitable<boost::beast::http::message_generator> person_day_off::get(session_data_ptr in_handle) {
  co_return in_handle->make_msg(nlohmann::json::object());
}

boost::asio::awaitable<boost::beast::http::message_generator> person_day_off_all::get(session_data_ptr in_handle) {
  co_return in_handle->make_msg(nlohmann::json::array());
}
boost::asio::awaitable<boost::beast::http::message_generator> person_time_spents_day_table::get(
    session_data_ptr in_handle
) {
  co_return in_handle->make_msg(nlohmann::json::array());
}
boost::asio::awaitable<boost::beast::http::message_generator> person_day_off_1::get(session_data_ptr in_handle) {
  co_return in_handle->make_msg(nlohmann::json::array());
}

}  // namespace doodle::http
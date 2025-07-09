//
// Created by TD on 25-4-3.
//
#include <doodle_lib/http_method/kitsu/kitsu_reg_url.h>

namespace doodle::http {
boost::asio::awaitable<boost::beast::http::message_generator> data_user_time_spents_all_get::callback_arg(
    session_data_ptr in_handle
) {
  co_return in_handle->make_msg(nlohmann::json::array());
}

boost::asio::awaitable<boost::beast::http::message_generator> data_user_time_spents_get::callback_arg(
    session_data_ptr in_handle, const std::shared_ptr<data_user_time_spents_arg>& in_arg
) {
  co_return in_handle->make_msg(nlohmann::json::array());
}
boost::asio::awaitable<boost::beast::http::message_generator> person_day_off_get::callback_arg(
    session_data_ptr in_handle, const std::shared_ptr<data_user_time_spents_arg>& in_arg
) {
  co_return in_handle->make_msg("{}");
}

boost::asio::awaitable<boost::beast::http::message_generator> person_day_off_all_get::callback_arg(
    session_data_ptr in_handle, const std::shared_ptr<capture_id_t>& in_arg
) {
  co_return in_handle->make_msg(nlohmann::json::array());
}
boost::asio::awaitable<boost::beast::http::message_generator> person_time_spents_day_table_get::callback_arg(
    session_data_ptr in_handle, const std::shared_ptr<year_month_arg>& in_arg
) {
  co_return in_handle->make_msg(nlohmann::json::array());
}
boost::asio::awaitable<boost::beast::http::message_generator> person_day_off_1_get::callback_arg(
    session_data_ptr in_handle, const std::shared_ptr<year_month_arg>& in_arg
) {
  co_return in_handle->make_msg(nlohmann::json::array());
}

}  // namespace doodle::http
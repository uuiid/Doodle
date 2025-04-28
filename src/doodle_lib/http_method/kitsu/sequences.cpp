//
// Created by TD on 25-4-28.
//
#include <doodle_lib/http_method/kitsu/kitsu_reg_url.h>

namespace doodle::http {
boost::asio::awaitable<boost::beast::http::message_generator> sequences_with_tasks_get::callback(
    session_data_ptr in_handle
) {
  co_return in_handle->make_msg("{}");
}

}  // namespace doodle::http
#include "ai_main.h"
#include <cache.hpp>
#include <cache_policy.hpp>
#include <lru_cache_policy.hpp>
#include <opencv2/opencv.hpp>

namespace doodle::http {
boost::asio::awaitable<boost::beast::http::message_generator> ai_train_binding_weights::post(
    session_data_ptr in_handle
) {
  co_return in_handle->make_error_code_msg(boost::beast::http::status::not_found, "服务器端未实现 api");
}
}  // namespace doodle::http
#include <doodle_core/core/global_function.h>
#include <doodle_core/metadata/ai_image_metadata.h>
#include <doodle_core/sqlite_orm/sqlite_database.h>

#include <doodle_lib/http_method/model_library/model_library.h>

#include <memory>
#include <spdlog/spdlog.h>

namespace doodle::http::model_library {
boost::asio::awaitable<boost::beast::http::message_generator> ai_image::get(session_data_ptr in_handle) {
  auto l_sql  = g_ctx().get<sqlite_database>();
  auto l_list = l_sql.get_all<ai_image_metadata>();
  co_return in_handle->make_msg(nlohmann::json{} = l_list);
}
boost::asio::awaitable<boost::beast::http::message_generator> ai_image::post(session_data_ptr in_handle) {
  auto l_sql              = g_ctx().get<sqlite_database>();
  auto l_ai_image         = std::make_shared<ai_image_metadata>();
  l_ai_image->author_     = person_.person_.uuid_id_;
  in_handle->get_json().get_to(*l_ai_image);
  co_await l_sql.install(l_ai_image);
  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 创建 AI 图片记录 {} ", person_.person_.email_,
      person_.person_.get_full_name(), l_ai_image->uuid_id_
  );
  co_return in_handle->make_msg(nlohmann::json{} = *l_ai_image);
}

boost::asio::awaitable<boost::beast::http::message_generator> ai_image_instance::delete_(session_data_ptr in_handle) {
  auto l_sql = g_ctx().get<sqlite_database>();
  co_await l_sql.remove<ai_image_metadata>(id_);
  co_return in_handle->make_msg(nlohmann::json{} = id_);
}

}  // namespace doodle::http::model_library
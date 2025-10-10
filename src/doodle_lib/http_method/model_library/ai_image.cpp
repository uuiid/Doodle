#include <doodle_core/metadata/ai_image_metadata.h>
#include <doodle_core/sqlite_orm/sqlite_database.h>

#include <doodle_lib/http_method/model_library/model_library.h>

#include <memory>

namespace doodle::http::model_library {
boost::asio::awaitable<boost::beast::http::message_generator> ai_image::get(session_data_ptr in_handle) {
  auto l_sql  = g_ctx().get<sqlite_database>();
  auto l_list = l_sql.get_all<ai_image_metadata>();
  co_return in_handle->make_msg(nlohmann::json{} = l_list);
}
boost::asio::awaitable<boost::beast::http::message_generator> ai_image_instance::post(session_data_ptr in_handle) {
  auto l_sql              = g_ctx().get<sqlite_database>();
  auto l_ai_image         = std::make_shared<ai_image_metadata>();
  l_ai_image->uuid_id_    = core_set::get_set().get_uuid();
  l_ai_image->created_at_ = chrono::system_zoned_time{chrono::current_zone(), chrono::system_clock::now()};
  l_ai_image->author_     = person_.person_.uuid_id_;
  in_handle->get_json().get_to(*l_ai_image);
  co_await l_sql.install(l_ai_image);
  co_return in_handle->make_msg(nlohmann::json{} = *l_ai_image);
}

boost::asio::awaitable<boost::beast::http::message_generator> ai_image_instance::delete_(session_data_ptr in_handle) {
  auto l_sql = g_ctx().get<sqlite_database>();
  co_await l_sql.remove<ai_image_metadata>(id_);
  co_return in_handle->make_msg(nlohmann::json{} = id_);
}

}  // namespace doodle::http::model_library
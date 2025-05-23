//
// Created by TD on 25-5-22.
//
#include "doodle_core/metadata/attachment_file.h"
#include "doodle_core/sqlite_orm/sqlite_database.h"

#include <doodle_lib/http_method/kitsu/kitsu_reg_url.h>
namespace doodle::http {
boost::asio::awaitable<boost::beast::http::message_generator> data_attachment_files_file_get::callback(
    session_data_ptr in_handle
) {
  auto l_sql             = g_ctx().get<sqlite_database>();
  auto l_attachment_file = l_sql.get_by_uuid<attachment_file>(in_handle->capture_->get_uuid());
  co_return in_handle->make_msg(nlohmann::json{});
}

}  // namespace doodle::http
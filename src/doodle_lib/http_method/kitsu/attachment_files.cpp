//
// Created by TD on 25-5-22.
//
#include "doodle_core/metadata/attachment_file.h"
#include "doodle_core/metadata/comment.h"
#include "doodle_core/metadata/task.h"
#include "doodle_core/sqlite_orm/sqlite_database.h"

#include "doodle_lib/http_method/kitsu.h"
#include <doodle_lib/http_method/kitsu/kitsu_reg_url.h>
namespace doodle::http {
boost::asio::awaitable<boost::beast::http::message_generator> data_attachment_files_file::get(
    session_data_ptr in_handle
) {
  auto l_sql             = g_ctx().get<sqlite_database>();
  auto l_attachment_file = l_sql.get_by_uuid<attachment_file>(id_);

  if (l_attachment_file.comment_id_.is_nil() && l_attachment_file.chat_message_id_.is_nil())
    throw_exception(http_request_error{boost::beast::http::status::not_found, "未找到对应的文件"});
  if (!l_attachment_file.comment_id_.is_nil()) {
    auto l_task = l_sql.get_by_uuid<task>(l_sql.get_by_uuid<comment>(l_attachment_file.comment_id_).object_id_);
    person_.check_project_manager(l_task.project_id_);
  }
  auto l_path = g_ctx().get<kitsu_ctx_t>().root_ / "files" / "attachments" /
                FSys::split_uuid_path(fmt::to_string(l_attachment_file.uuid_id_));
  auto l_ext = l_attachment_file.extension_;
  co_return in_handle->make_msg(l_path, http_header_ctrl{.mine_type_ = kitsu::mime_type(l_ext)});
}

}  // namespace doodle::http
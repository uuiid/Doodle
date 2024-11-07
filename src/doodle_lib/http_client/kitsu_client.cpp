#include "kitsu_client.h"

#include <doodle_core/metadata/kitsu/task_type.h>
namespace doodle::kitsu {
boost::asio::awaitable<std::tuple<boost::system::error_code, kitsu_client::task>> kitsu_client::get_task(
    const boost::uuids::uuid& in_uuid
) {
  boost::beast::http::request<boost::beast::http::empty_body> req{
      boost::beast::http::verb::get, fmt::format("/api/data/tasks/{}/full", in_uuid), 11
  };

  auto [l_e, l_res] = co_await http::detail::read_and_write<boost::beast::http::string_body>(
      http_client_core_ptr_, header_operator_req(std::move(req))
  );
  if (l_e) {
    co_return std::make_tuple(l_e, task{});
  }
  header_operator_resp(l_res);
  task l_task{};
  try {
    l_task = nlohmann::json::parse(l_res.body()).get<task>();
  } catch (const nlohmann::json::exception& e) {
    http_client_core_ptr_->logger_->log(log_loc(), level::err, "get task failed: {}", e.what());
    co_return std::make_tuple(boost::system::errc::make_error_code(boost::system::errc::invalid_argument), l_task);
  }
  co_return std::make_tuple(l_e, l_task);
}

boost::asio::awaitable<tl::expected<kitsu_client::user_t, std::string>> kitsu_client::get_user(
    const boost::uuids::uuid& in_uuid
) {
  boost::beast::http::request<boost::beast::http::empty_body> req{
      boost::beast::http::verb::get, fmt::format("/api/data/persons/{}", in_uuid), 11
  };

  user_t l_user;
  auto [l_e, l_res] = co_await http::detail::read_and_write<boost::beast::http::string_body>(
      http_client_core_ptr_, header_operator_req(std::move(req))
  );
  if (l_e) {
    co_return tl::make_unexpected(fmt::format("无法获取到用户电话号码 {}", l_e));
  }
  header_operator_resp(l_res);
  try {
    l_user = nlohmann::json::parse(l_res.body()).get<user_t>();
  } catch (const nlohmann::json::exception& e) {
    http_client_core_ptr_->logger_->log(log_loc(), level::err, "get user failed: {}", e.what());
    co_return tl::make_unexpected(fmt::format("无法获取到用户电话号码 {}", e.what()));
  }
  co_return l_user.phone_.empty()
      ? tl::expected<kitsu_client::user_t, std::string>{tl::make_unexpected("用户电话号码为空")}
      : tl::expected<kitsu_client::user_t, std::string>{l_user};
}

boost::asio::awaitable<tl::expected<std::vector<project_helper::database_t>, std::string>>
kitsu_client::get_all_project() {
  auto&& [l_e, l_res] = co_await http::detail::read_and_write<boost::beast::http::string_body>(
      http_client_core_ptr_, header_operator_req(boost::beast::http::request<boost::beast::http::empty_body>{
                                 boost::beast::http::verb::get, "/api/data/projects/all", 11
                             })
  );

  tl::expected<std::vector<project_helper::database_t>, std::string> l_ret{};
  if (l_e)
    l_ret = tl::make_unexpected(l_e.what());
  else {
    header_operator_resp(l_res);
    std::vector<project_helper::database_t> l_list{};
    for (auto&& l_prj : nlohmann::json::parse(l_res.body())) {
      l_list
          .emplace_back(project_helper::database_t{
              .uuid_id_          = l_prj["id"].get<uuid>(),
              .name_             = l_prj["name"].get<std::string>(),
              .path_             = "C:/sy",
              .local_path_       = "C:/sy",
              .auto_upload_path_ = "C:/sy"
          })
          .generate_names();
    }
    l_ret = std::move(l_list);
  }
  co_return l_ret;
}
boost::asio::awaitable<tl::expected<std::vector<metadata::kitsu::task_type_t>, std::string>>
kitsu_client::get_all_task_type() {
  auto&& [l_e, l_res] = co_await http::detail::read_and_write<boost::beast::http::string_body>(
      http_client_core_ptr_, header_operator_req(boost::beast::http::request<boost::beast::http::empty_body>{
                                 boost::beast::http::verb::get, "/api/data/task-types", 11
                             })
  );
  tl::expected<std::vector<metadata::kitsu::task_type_t>, std::string> l_ret{};
  if (l_e)
    l_ret = tl::make_unexpected(l_e.what());
  else {
    header_operator_resp(l_res);
    std::vector<metadata::kitsu::task_type_t> l_list{};
    for (auto&& l_prj : nlohmann::json::parse(l_res.body())) {
      l_list.emplace_back(metadata::kitsu::task_type_t{
          .uuid_id_    = l_prj["id"].get<uuid>(),
          .name_       = l_prj["name"].get<std::string>(),
      });
    }
    l_ret = std::move(l_list);
  }
  co_return l_ret;
}

}  // namespace doodle::kitsu
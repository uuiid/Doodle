//
// Created by TD on 2024/2/27.
//

#include "task_info.h"

#include "doodle_core/core/app_base.h"
#include "doodle_core/sqlite_orm/sqlite_database.h"
#include <doodle_core/lib_warp/boost_uuid_warp.h>
#include <doodle_core/lib_warp/json_warp.h>
#include <doodle_core/metadata/server_task_info.h>

#include <doodle_lib/core/http/http_session_data.h>
#include <doodle_lib/core/http/json_body.h>
#include <doodle_lib/exe_warp/import_and_render_ue.h>
#include <doodle_lib/exe_warp/maya_exe.h>
#include <doodle_lib/exe_warp/ue_exe.h>
#include <doodle_lib/http_method/computer_reg_data.h>
#include <doodle_lib/http_method/kitsu/kitsu.h>
#include <doodle_lib/long_task/connect_video.h>
#include <doodle_lib/long_task/image_to_move.h>

#include <spdlog/sinks/basic_file_sink.h>

namespace doodle::http {
namespace {
boost::asio::awaitable<void> task_emit(const std::shared_ptr<server_task_info>& in_ptr) {
  auto l_computer_list = computer_reg_data_manager::get().list();
  if (l_computer_list.empty()) co_return;
  for (auto&& l_com : l_computer_list) {
    if (l_com->computer_data_ptr_->uuid_id_ == in_ptr->run_computer_id_)
      if (auto l_c = l_com->client.lock(); l_c) {
        co_await l_c->async_write_websocket(
            nlohmann::json{{"type", doodle_config::work_websocket_event::post_task}, {"id", in_ptr->uuid_id_}}.dump()
        );
        co_return;
      }
  }
}

boost::asio::awaitable<boost::beast::http::message_generator> post_task(session_data_ptr in_handle) {
  auto l_logger = in_handle->logger_;
  if (in_handle->content_type_ != http::detail::content_type::application_json)
    co_return in_handle->make_error_code_msg(
        boost::beast::http::status::bad_request, boost::system::errc::make_error_code(boost::system::errc::bad_message),
        "不是json请求"
    );
  auto l_ptr  = std::make_shared<server_task_info>();

  auto l_json = std::get<nlohmann::json>(in_handle->body_);
  l_json.get_to(*l_ptr);
  l_ptr->uuid_id_     = core_set::get_set().get_uuid();
  l_ptr->submit_time_ = chrono::sys_time_pos::clock::now();

  if (l_ptr->exe_.empty())
    co_return in_handle->make_error_code_msg(boost::beast::http::status::bad_request, "运行程序任务为空");

  if (auto l_list = g_ctx().get<sqlite_database>().uuid_to_id<computer>(l_ptr->run_computer_id_); l_list == 0)
    co_return in_handle->make_error_code_msg(boost::beast::http::status::not_found, "未找到计算机");

  co_await g_ctx().get<sqlite_database>().install(l_ptr);

  boost::asio::co_spawn(g_io_context(), task_emit(l_ptr), boost::asio::consign(boost::asio::detached, l_ptr));
  co_return in_handle->make_msg((nlohmann::json{} = *l_ptr).dump());
}

boost::asio::awaitable<boost::beast::http::message_generator> get_task(session_data_ptr in_handle) {
  uuid l_uuid = boost::lexical_cast<boost::uuids::uuid>(in_handle->capture_->get("id"));

  if (auto l_list = g_ctx().get<sqlite_database>().get_by_uuid<server_task_info>(l_uuid); !l_list.empty()) {
    l_list[0].get_last_line_log();
    co_return in_handle->make_msg((nlohmann::json{} = l_list[0]).dump());
  }

  co_return in_handle->make_error_code_msg(boost::beast::http::status::not_found, "任务不存在");
}

boost::asio::awaitable<boost::beast::http::message_generator> list_task(session_data_ptr in_handle) {
  std::optional<server_task_info_type> l_type{};
  for (auto&& i : in_handle->url_.params()) {
    if (i.has_value && i.key == "type") {
      l_type = magic_enum::enum_cast<server_task_info_type>(i.value);
    }
  }
  if (l_type) {
    if (auto l_list = g_ctx().get<sqlite_database>().get_server_task_info_by_type(*l_type); !l_list.empty())
      co_return in_handle->make_msg((nlohmann::json{} = l_list).dump());
    co_return in_handle->make_msg("[]"s);
  }

  co_return in_handle->make_msg((nlohmann::json{} = g_ctx().get<sqlite_database>().get_all<server_task_info>()).dump());
}

boost::asio::awaitable<boost::beast::http::message_generator> get_task_logger(session_data_ptr in_handle) {
  boost::uuids::uuid l_uuid{boost::lexical_cast<boost::uuids::uuid>(in_handle->capture_->get("id"))};

  auto l_path =
      core_set::get_set().get_cache_root() / server_task_info::logger_category / fmt::format("{}.log", l_uuid);
  if (!FSys::exists(l_path))
    co_return in_handle->make_error_code_msg(boost::beast::http::status::not_found, "日志不存在");
  auto l_mime = std::string{kitsu::mime_type(l_path.extension())};
  l_mime += "; charset=utf-8";
  auto l_ex = in_handle->make_msg(l_path, l_mime);
  if (!l_ex) co_return in_handle->make_error_code_msg(boost::beast::http::status::internal_server_error, l_ex.error());
  co_return std::move(*l_ex);
}
boost::asio::awaitable<boost::beast::http::message_generator> get_task_logger_mini(session_data_ptr in_handle) {
  boost::uuids::uuid l_uuid{boost::lexical_cast<boost::uuids::uuid>(in_handle->capture_->get("id"))};

  auto l_path =
      core_set::get_set().get_cache_root() / server_task_info::logger_category / fmt::format("{}.log", l_uuid);
  if (!FSys::exists(l_path))
    co_return in_handle->make_error_code_msg(boost::beast::http::status::not_found, "日志不存在");
  auto l_mime = std::string{kitsu::mime_type(l_path.extension())};
  l_mime += "; charset=utf-8";

  FSys::ifstream l_ifs(l_path, std::ios::binary | std::ios::ate);
  auto l_size = l_ifs.tellg();
  if (l_size > 5100) {
    l_size -= 5000;
    l_ifs.seekg(l_size);
  } else
    l_ifs.seekg(0);
  std::string l_content{};
  l_content.resize(5000ull, '\0');
  l_ifs.read(l_content.data(), 5000);
  co_return in_handle->make_msg(std::move(l_content), l_mime);
}
boost::asio::awaitable<boost::beast::http::message_generator> delete_task(session_data_ptr in_handle) {
  auto l_uuid = std::make_shared<uuid>(boost::lexical_cast<boost::uuids::uuid>(in_handle->capture_->get("id")));

  if (auto l_list = g_ctx().get<sqlite_database>().get_by_uuid<server_task_info>(*l_uuid);
      !l_list.empty() && l_list.front().status_ == server_task_info_status::running) {
    co_return in_handle->make_error_code_msg(
        boost::beast::http::status::method_not_allowed, "任务正在运行中, 无法删除"
    );
  }
  co_await g_ctx().get<sqlite_database>().remove<server_task_info>(l_uuid);
  co_return in_handle->make_msg("{}");
}

template <class Mutex>
class run_post_task_local_impl_sink : public spdlog::sinks::base_sink<Mutex> {
  std::shared_ptr<server_task_info> task_info_{};
  std::once_flag flag_;

 public:
  explicit run_post_task_local_impl_sink(std::shared_ptr<server_task_info> in_task_info) : task_info_(in_task_info) {}
  void sink_it_(const spdlog::details::log_msg& msg) override {
    // std::call_once(flag_, &set_state, this);
    std::call_once(flag_, [this]() { set_state(); });
  }
  void flush_() override {}
  void set_state() {
    task_info_->status_   = server_task_info_status::running;
    task_info_->run_time_ = server_task_info::zoned_time{chrono::current_zone(), std::chrono::system_clock::now()};
    boost::asio::co_spawn(g_io_context(), g_ctx().get<sqlite_database>().install(task_info_), boost::asio::detached);
  }
};
using run_post_task_local_impl_sink_mt = run_post_task_local_impl_sink<std::mutex>;

class run_post_task_local_cancel_manager {
  std::map<uuid, std::size_t> sigs_index;
  std::vector<std::shared_ptr<boost::asio::cancellation_signal>> sigs_{};
  std::recursive_mutex mtx_;

 public:
  run_post_task_local_cancel_manager() = default;

  boost::asio::cancellation_slot add(uuid in_id) {
    std::lock_guard<std::recursive_mutex> _(mtx_);
    auto itr = std::find_if(sigs_.begin(), sigs_.end(), [](std::shared_ptr<boost::asio::cancellation_signal>& sig) {
      return !sig->slot().has_handler();
    });
    if (itr != sigs_.end()) {
      sigs_index[in_id] = std::distance(sigs_.begin(), itr);
      return (*itr)->slot();
    } else {
      sigs_index[in_id] = sigs_.size();
      return sigs_.emplace_back(std::make_shared<boost::asio::cancellation_signal>())->slot();
    }
  }

  void cancel(uuid in_id) {
    std::lock_guard<std::recursive_mutex> _(mtx_);
    if (auto itr = sigs_index.find(in_id);
        itr != sigs_index.end() && itr->second < sigs_.size() && sigs_[itr->second]->slot().has_handler())
      sigs_[itr->second]->emit(boost::asio::cancellation_type::all);
  }
  void cancel_all() {
    std::lock_guard<std::recursive_mutex> _(mtx_);
    for (auto& sig : sigs_) sig->emit(boost::asio::cancellation_type::all);
  }
};

class run_long_task_local : public std::enable_shared_from_this<run_long_task_local> {
  std::variant<
      std::shared_ptr<maya_exe_ns::arg>, std::shared_ptr<import_and_render_ue_ns::args>,
      std::shared_ptr<doodle::detail::image_to_move>, std::shared_ptr<doodle::detail::connect_video_t>>
      arg_;
  logger_ptr logger_{};

 public:
  std::shared_ptr<server_task_info> task_info_{};
  run_long_task_local() = default;
  explicit run_long_task_local(std::shared_ptr<server_task_info> in_task_info) : task_info_(std::move(in_task_info)) {}

  void load_form_json(const nlohmann::json& in_json) {
    if (in_json.contains("replace_ref_file")) {  /// 解算文件
      auto l_arg_t = std::make_shared<maya_exe_ns::qcloth_arg>();
      in_json.get_to(*l_arg_t);
      l_arg_t->sim_path = FSys::path{in_json["project"]["path"].get<std::string>()} / "6-moxing" / "CFX";
      arg_              = l_arg_t;
    } else if (in_json.contains("file_list")) {  /// 替换文件
      auto l_arg_t = std::make_shared<maya_exe_ns::replace_file_arg>();
      in_json.get_to(*l_arg_t);
      arg_ = l_arg_t;
    } else if (in_json.contains("is_sim")) {  /// 自动渲染
      auto l_import_and_render_args = std::make_shared<import_and_render_ue_ns::args>();
      in_json.get_to(*l_import_and_render_args);
      if (in_json["is_sim"].get<bool>()) {
        auto l_arg_t = std::make_shared<maya_exe_ns::qcloth_arg>();
        in_json.get_to(*l_arg_t);
        l_arg_t->sim_path                   = l_import_and_render_args->project_.path_ / "6-moxing" / "CFX";
        l_arg_t->export_file                = true;
        l_arg_t->touch_sim                  = true;
        l_arg_t->export_anim_file           = true;
        l_arg_t->create_play_blast_         = true;
        l_import_and_render_args->maya_arg_ = l_arg_t;
      } else {
        auto l_arg_t = std::make_shared<maya_exe_ns::export_fbx_arg>();
        in_json.get_to(*l_arg_t);
        l_arg_t->create_play_blast_         = true;
        l_import_and_render_args->maya_arg_ = l_arg_t;
      }
      arg_ = l_import_and_render_args;
    } else if (in_json.contains("category")) {  // 检查文件任务
      auto l_arg_t = std::make_shared<maya_exe_ns::inspect_file_arg>();
      l_arg_t->config(in_json["category"].get<maya_exe_ns::inspect_file_type>());
      l_arg_t->file_path = in_json["path"].get<std::string>();
      arg_               = l_arg_t;
    } else if (in_json.contains("image_to_move")) {
      auto l_image_to_move_args = std::make_shared<doodle::detail::image_to_move>();
      in_json.get_to(*l_image_to_move_args);
      arg_ = l_image_to_move_args;
    } else if (in_json.contains("connect_video")) {
      auto l_connect_video_args = std::make_shared<doodle::detail::connect_video_t>();
      in_json.get_to(*l_connect_video_args);
      arg_ = l_connect_video_args;
    } else {  /// 导出fbx
      auto l_arg_t = std::make_shared<maya_exe_ns::export_fbx_arg>();
      in_json.get_to(*l_arg_t);
      arg_ = l_arg_t;
    }
    auto l_logger_path = core_set::get_set().get_cache_root() / server_task_info::logger_category /
                         fmt::format("{}.log", task_info_->uuid_id_);
    logger_ = std::make_shared<spdlog::async_logger>(
        task_info_->name_, std::make_shared<spdlog::sinks::basic_file_sink_mt>(l_logger_path.generic_string()),
        spdlog::thread_pool()
    );
    logger_->sinks().emplace_back(std::make_shared<run_post_task_local_impl_sink_mt>(task_info_));
  }

  void run() {
    boost::asio::co_spawn(
        g_io_context(), std::visit(*this, arg_),
        boost::asio::bind_cancellation_slot(
            g_ctx().get<run_post_task_local_cancel_manager>().add(task_info_->uuid_id_),
            boost::asio::consign(boost::asio::detached, shared_from_this())
        )
    );
  }

  boost::asio::awaitable<void> operator()(std::shared_ptr<maya_exe_ns::arg>& in_arg) const {
    auto [l_e, l_r]       = co_await async_run_maya(in_arg, logger_);
    task_info_->end_time_ = server_task_info::zoned_time{chrono::current_zone(), std::chrono::system_clock::now()};
    // 用户取消
    if ((co_await boost::asio::this_coro::cancellation_state).cancelled() != boost::asio::cancellation_type::none)
      task_info_->status_ = server_task_info_status::canceled;
    else if (l_e) {
      task_info_->status_ = server_task_info_status::failed;
    } else
      task_info_->status_ = server_task_info_status::completed;
    boost::asio::co_spawn(
        g_io_context(), g_ctx().get<sqlite_database>().install(task_info_),
        boost::asio::bind_cancellation_slot(
            app_base::Get().on_cancel.slot(), boost::asio::detached

        )
    );
    co_return;
  }
  boost::asio::awaitable<void> operator()(std::shared_ptr<import_and_render_ue_ns::args>& in_arg) const {
    auto [l_e, l_r]       = co_await async_auto_loght(in_arg, logger_);
    task_info_->end_time_ = server_task_info::zoned_time{chrono::current_zone(), std::chrono::system_clock::now()};
    // 用户取消
    if ((co_await boost::asio::this_coro::cancellation_state).cancelled() != boost::asio::cancellation_type::none)
      task_info_->status_ = server_task_info_status::canceled;
    else if (l_e) {
      task_info_->status_ = server_task_info_status::failed;
    } else
      task_info_->status_ = server_task_info_status::completed;
    boost::asio::co_spawn(
        g_io_context(), g_ctx().get<sqlite_database>().install(task_info_),
        boost::asio::bind_cancellation_slot(
            app_base::Get().on_cancel.slot(), boost::asio::detached

        )
    );
    co_return;
  }
  boost::asio::awaitable<void> operator()(std::shared_ptr<doodle::detail::image_to_move>& in_arg) const {
    co_await boost::asio::post(g_io_context(), boost::asio::use_awaitable);
    std::vector<FSys::path> l_paths{};
    for (auto&& l_path_info : FSys::directory_iterator{in_arg->path_}) {
      auto l_ext = l_path_info.path().extension();
      if (l_ext == ".png" || l_ext == ".exr" || l_ext == ".jpg") l_paths.emplace_back(l_path_info.path());
    }
    auto l_images = doodle::movie::image_attr::make_default_attr(&in_arg->eps_, &in_arg->shot_, l_paths);
    for (auto&& l_image : l_images) {
      l_image.watermarks_attr.emplace_back(in_arg->user_name_, 0.7, 0.2, movie::image_watermark::rgb_default);
    }
    auto l_ec             = doodle::detail::create_move(in_arg->out_path_, logger_, l_images, in_arg->image_size_);
    task_info_->end_time_ = server_task_info::zoned_time{chrono::current_zone(), std::chrono::system_clock::now()};
    // 用户取消
    if ((co_await boost::asio::this_coro::cancellation_state).cancelled() != boost::asio::cancellation_type::none)
      task_info_->status_ = server_task_info_status::canceled;
    else if (l_ec) {
      task_info_->status_ = server_task_info_status::failed;
    } else
      task_info_->status_ = server_task_info_status::completed;
    boost::asio::co_spawn(
        g_io_context(), g_ctx().get<sqlite_database>().install(task_info_),
        boost::asio::bind_cancellation_slot(
            app_base::Get().on_cancel.slot(), boost::asio::detached

        )
    );
  }
  boost::asio::awaitable<void> operator()(std::shared_ptr<doodle::detail::connect_video_t>& in_arg) const {
    co_await boost::asio::post(g_io_context(), boost::asio::use_awaitable);
    in_arg->file_list_ |=
        ranges::action::sort([](const FSys::path& l_a, const FSys::path& l_b) { return l_a.stem() < l_b.stem(); });
    auto l_ec = doodle::detail::connect_video(in_arg->out_path_, logger_, in_arg->file_list_, in_arg->image_size_);
    task_info_->end_time_ = server_task_info::zoned_time{chrono::current_zone(), std::chrono::system_clock::now()};
    // 用户取消
    if ((co_await boost::asio::this_coro::cancellation_state).cancelled() != boost::asio::cancellation_type::none)
      task_info_->status_ = server_task_info_status::canceled;
    else if (l_ec) {
      task_info_->status_ = server_task_info_status::failed;
    } else
      task_info_->status_ = server_task_info_status::completed;
    boost::asio::co_spawn(
        g_io_context(), g_ctx().get<sqlite_database>().install(task_info_),
        boost::asio::bind_cancellation_slot(
            app_base::Get().on_cancel.slot(), boost::asio::detached

        )
    );
  }
};

boost::asio::awaitable<boost::beast::http::message_generator> post_task_local(session_data_ptr in_handle) {
  auto l_logger = in_handle->logger_;
  if (in_handle->content_type_ != http::detail::content_type::application_json)
    co_return in_handle->make_error_code_msg(
        boost::beast::http::status::bad_request, boost::system::errc::make_error_code(boost::system::errc::bad_message),
        "不是json请求"
    );
  auto l_ptr = std::make_shared<server_task_info>();
  std::shared_ptr<maya_exe_ns::arg> l_arg{};
  std::shared_ptr<import_and_render_ue_ns::args> l_import_and_render_args{};
  std::shared_ptr<doodle::detail::image_to_move> l_image_to_move_args{};
  std::shared_ptr<doodle::detail::connect_video_t> l_connect_video_args{};

  logger_ptr l_logger_ptr{};

  auto l_json = std::get<nlohmann::json>(in_handle->body_);
  l_json.get_to(*l_ptr);
  l_ptr->uuid_id_         = core_set::get_set().get_uuid();
  l_ptr->submit_time_     = server_task_info::zoned_time{chrono::current_zone(), std::chrono::system_clock::now()};
  l_ptr->run_computer_id_ = boost::uuids::nil_uuid();
  if (l_ptr->name_.empty()) l_ptr->name_ = fmt::to_string(l_ptr->uuid_id_);

  auto& l_task               = l_json["task_data"];
  auto l_run_long_task_local = std::make_shared<run_long_task_local>(l_ptr);
  // 先进行数据加载, 如果出错抛出异常后直接不插入数据库
  l_run_long_task_local->load_form_json(l_task);
  co_await g_ctx().get<sqlite_database>().install(l_ptr);
  l_run_long_task_local->run();
  co_return in_handle->make_msg((nlohmann::json{} = *l_ptr).dump());
}

boost::asio::awaitable<boost::beast::http::message_generator> patch_task_local(session_data_ptr in_handle) {
  auto l_uuid = std::make_shared<uuid>(boost::lexical_cast<boost::uuids::uuid>(in_handle->capture_->get("id")));
  auto l_server_task_info = std::make_shared<server_task_info>();

  server_task_info l_server_task_info_org{};
  if (auto l_list = g_ctx().get<sqlite_database>().get_by_uuid<server_task_info>(*l_uuid); l_list.empty()) {
    co_return in_handle->make_error_code_msg(boost::beast::http::status::bad_request, "任务不存在");
  } else {
    *l_server_task_info    = l_list[0];
    l_server_task_info_org = l_list[0];
  }
  auto l_sr   = l_server_task_info->status_;
  auto l_json = std::get<nlohmann::json>(in_handle->body_);
  if (l_json.contains("status")) l_json["status"].get_to(l_server_task_info->status_);
  if (l_json.contains("name")) l_json["name"].get_to(l_server_task_info->name_);
  if (*l_server_task_info != l_server_task_info_org)
    co_await g_ctx().get<sqlite_database>().install(l_server_task_info);
  if (l_sr == server_task_info_status::running && l_server_task_info->status_ == server_task_info_status::canceled)
    g_ctx().get<run_post_task_local_cancel_manager>().cancel(l_server_task_info->uuid_id_);
  co_return in_handle->make_msg((nlohmann::json{} = *l_server_task_info).dump());
}

}  // namespace

void task_info_reg(doodle::http::http_route& in_route) {
  in_route.reg(std::make_shared<http_function>(boost::beast::http::verb::get, "api/doodle/task", list_task))
      .reg(std::make_shared<http_function>(boost::beast::http::verb::get, "api/doodle/task/{id}", get_task))
      .reg(std::make_shared<http_function>(boost::beast::http::verb::get, "api/doodle/task/{id}/log", get_task_logger))
      .reg(std::make_shared<http_function>(boost::beast::http::verb::post, "api/doodle/task", post_task))
      .reg(std::make_shared<http_function>(boost::beast::http::verb::delete_, "api/doodle/task/{id}", delete_task));
}
void task_info_reg_local(doodle::http::http_route& in_route) {
  if (!g_ctx().contains<maya_ctx>()) g_ctx().emplace<maya_ctx>();
  if (!g_ctx().contains<ue_ctx>()) g_ctx().emplace<ue_ctx>();
  g_ctx().emplace<run_post_task_local_cancel_manager>();
  app_base::Get().on_stop.connect([]() { g_ctx().get<run_post_task_local_cancel_manager>().cancel_all(); });

  in_route.reg(std::make_shared<http_function>(boost::beast::http::verb::get, "api/doodle/task", list_task))
      .reg(std::make_shared<http_function>(boost::beast::http::verb::get, "api/doodle/task/{id}", get_task))
      .reg(std::make_shared<http_function>(boost::beast::http::verb::get, "api/doodle/task/{id}/log", get_task_logger))
      .reg(
          std::make_shared<http_function>(
              boost::beast::http::verb::get, "api/doodle/task/{id}/log/mini", get_task_logger_mini
          )
      )
      .reg(std::make_shared<http_function>(boost::beast::http::verb::post, "api/doodle/task", post_task_local))
      .reg(std::make_shared<http_function>(boost::beast::http::verb::patch, "api/doodle/task/{id}", patch_task_local))
      .reg(std::make_shared<http_function>(boost::beast::http::verb::delete_, "api/doodle/task/{id}", delete_task));
}
}  // namespace doodle::http
//
// Created by TD on 2024/2/27.
//

#include <doodle_core/lib_warp/boost_uuid_warp.h>
#include <doodle_core/lib_warp/json_warp.h>
#include <doodle_core/metadata/project.h>
#include <doodle_core/metadata/server_task_info.h>

#include <doodle_lib/core/app_base.h>
#include <doodle_lib/core/asyn_task.h>
#include <doodle_lib/core/core_set.h>
#include <doodle_lib/core/http/http_session_data.h>
#include <doodle_lib/core/http/json_body.h>
#include <doodle_lib/core/socket_io/broadcast.h>
#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/exe_warp/assets_update.h>
#include <doodle_lib/exe_warp/export_fbx_arg.h>
#include <doodle_lib/exe_warp/export_rig_sk.h>
#include <doodle_lib/exe_warp/folder_watcher_anim_fbx.h>
#include <doodle_lib/exe_warp/import_and_render_ue.h>
#include <doodle_lib/exe_warp/inspect_maya.h>
#include <doodle_lib/exe_warp/maya_exe.h>
#include <doodle_lib/exe_warp/qcloth_arg.h>
#include <doodle_lib/exe_warp/task_sync.h>
#include <doodle_lib/exe_warp/ue_exe.h>
#include <doodle_lib/http_client/kitsu_client.h>
#include <doodle_lib/http_method/computer_reg_data.h>
#include <doodle_lib/http_method/kitsu.h>
#include <doodle_lib/http_method/kitsu/epiboly.h>
#include <doodle_lib/http_method/local/local.h>
#include <doodle_lib/long_task/connect_video.h>
#include <doodle_lib/long_task/image_to_move.h>
#include <doodle_lib/sqlite_orm/detail/sqlite_database_impl.h>
#include <doodle_lib/sqlite_orm/sqlite_database.h>

#include <boost/asio/bind_cancellation_slot.hpp>
#include <boost/uuid/detail/nil_uuid.hpp>
#include <boost/uuid/uuid.hpp>

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <memory>
#include <spdlog/sinks/basic_file_sink.h>

namespace doodle::http::local {

namespace {

class task_info_manager {
 private:
  std::map<uuid, std::shared_ptr<server_task_info>> task_infos_;
  std::recursive_mutex mtx_;

 public:
  void add_task_info(const std::shared_ptr<server_task_info>& in_info) {
    std::lock_guard<std::recursive_mutex> _(mtx_);
    task_infos_[in_info->uuid_id_] = in_info;
  }

  std::shared_ptr<server_task_info> get_task_info(const uuid& in_id) {
    std::lock_guard<std::recursive_mutex> _(mtx_);
    auto itr = task_infos_.find(in_id);
    if (itr != task_infos_.end()) return itr->second;
    return nullptr;
  }
  std::vector<server_task_info> get_all_task_info() {
    std::lock_guard<std::recursive_mutex> _(mtx_);
    std::vector<server_task_info> result;
    for (const auto& [_, info_ptr] : task_infos_) {
      if (info_ptr) result.push_back(*info_ptr);
    }
    return result;
  }

  void remove_task_info(const uuid& in_id) {
    std::lock_guard<std::recursive_mutex> _(mtx_);
    task_infos_.erase(in_id);
  }

  task_info_manager& get_instance() {
    static task_info_manager instance;
    return instance;
  }
};

template <class Mutex>
class run_post_task_local_impl_sink : public spdlog::sinks::base_sink<Mutex> {
  std::shared_ptr<server_task_info> task_info_{};
  std::once_flag flag_;

 public:
  explicit run_post_task_local_impl_sink(std::shared_ptr<server_task_info> in_task_info) : task_info_(in_task_info) {}
  void sink_it_(const spdlog::details::log_msg& msg) override {
    std::call_once(flag_, [this]() { set_state(); });
    task_info_->progress_ += 0.001;
    if (task_info_->progress_ > 1.0) task_info_->progress_ = std::rand() % 100 / 500.0;
    if (static_cast<int>(task_info_->progress_ * 100) % 2 == 0 || task_info_->progress_ <= 0.1)
      socket_io::broadcast(
          socket_io::local_server_task_info_progress_broadcast_t{
              .id_ = task_info_->uuid_id_, .progress_ = task_info_->progress_
          }
      );
  }
  void flush_() override {}
  void set_state() {
    if (task_info_->status_ != server_task_info_status::submitted) return;

    task_info_->status_   = server_task_info_status::running;
    task_info_->run_time_ = server_task_info::zoned_time{chrono::current_zone(), std::chrono::system_clock::now()};
    socket_io::broadcast(socket_io::local_server_task_info_update_broadcast_t{.main_info_ = *task_info_});
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
    for (auto& sig : sigs_) {
      if (sig->slot().has_handler()) sig->emit(boost::asio::cancellation_type::all);
    }
  }
};

class run_long_task_local : public std::enable_shared_from_this<run_long_task_local> {
  using async_task_ptr = std::shared_ptr<async_task>;
  async_task_ptr arg_;
  logger_ptr logger_{};
  // 强制等待一秒
  boost::asio::awaitable<void> wait() const {
    auto l_timer = boost::asio::system_timer{co_await boost::asio::this_coro::executor};
    l_timer.expires_after(std::chrono::seconds(1));
    try {
      co_await l_timer.async_wait(boost::asio::use_awaitable);
    } catch (...) {
      default_logger_raw()->log(log_loc(), level::err, boost::current_exception_diagnostic_information());
    }
  }

 public:
  std::shared_ptr<server_task_info> task_info_{};
  run_long_task_local() = default;
  explicit run_long_task_local(std::shared_ptr<server_task_info> in_task_info) : task_info_(std::move(in_task_info)) {}

  void set_arg(const async_task_ptr& in_arg) {
    auto l_logger_path = core_set::get_set().get_cache_root() / server_task_info::logger_category /
                         fmt::format("{}.log", task_info_->uuid_id_);
    logger_ = std::make_shared<spdlog::async_logger>(
        task_info_->name_, std::make_shared<spdlog::sinks::basic_file_sink_mt>(l_logger_path.generic_string()),
        spdlog::thread_pool()
    );
    logger_->sinks().emplace_back(std::make_shared<run_post_task_local_impl_sink_mt>(task_info_));
    arg_ = in_arg;
    arg_->set_logger(logger_);
  }

  void load_form_json(const nlohmann::json& in_json) {
    auto l_logger_path = core_set::get_set().get_cache_root() / server_task_info::logger_category /
                         fmt::format("{}.log", task_info_->uuid_id_);
    logger_ = std::make_shared<spdlog::async_logger>(
        task_info_->name_, std::make_shared<spdlog::sinks::basic_file_sink_mt>(l_logger_path.generic_string()),
        spdlog::thread_pool()
    );
    logger_->sinks().emplace_back(std::make_shared<run_post_task_local_impl_sink_mt>(task_info_));

    if (in_json.contains("replace_ref_file")) {  /// 解算文件
      auto l_arg_t = std::make_shared<qcloth_arg>();
      in_json.get_to(*l_arg_t);
      l_arg_t->sim_path = FSys::path{in_json["project"]["path"].get<std::string>()} /
                          in_json["project"]["asset_root_path"].get<std::string>() / "CFX";
      arg_ = l_arg_t;
    } else if (in_json.contains("file_list")) {  /// 替换文件
      auto l_arg_t = std::make_shared<maya_exe_ns::replace_file_arg>();
      in_json.get_to(*l_arg_t);
      arg_ = l_arg_t;
    } else if (in_json.contains("image_to_move")) {  // 图片到视频
      auto l_image_to_move_args = std::make_shared<doodle::detail::image_to_move>();
      in_json.get_to(*l_image_to_move_args);
      arg_ = l_image_to_move_args;
    } else if (in_json.contains("connect_video")) {  // 连接视频
      auto l_connect_video_args = std::make_shared<doodle::detail::connect_video_t>();
      in_json.get_to(*l_connect_video_args);
      arg_ = l_connect_video_args;
    } else {  /// 导出fbx
      auto l_arg_t = std::make_shared<export_fbx_arg>();
      in_json.get_to(*l_arg_t);
      arg_ = l_arg_t;
    }
    arg_->set_logger(logger_);
  }

  void run() {
    // auto l_v = std::visit(*this, arg_);
    boost::asio::co_spawn(
        g_io_context(), (*this)(),
        boost::asio::bind_cancellation_slot(
            g_ctx().get<run_post_task_local_cancel_manager>().add(task_info_->uuid_id_),
            boost::asio::consign(boost::asio::detached, shared_from_this())
        )
    );
  }

  boost::asio::awaitable<void> operator()() const {
    co_await wait();
    try {
      emit_signal();
      co_await arg_->run();
      task_info_->status_ = server_task_info_status::completed;
    } catch (const boost::system::system_error& e) {
      if (e.code() == boost::system::errc::operation_canceled) task_info_->status_ = server_task_info_status::canceled;
      logger_->error("{}", e.what());
      task_info_->status_        = server_task_info_status::canceled;
      task_info_->last_line_log_ = fmt::format("{}", e.what());
    } catch (...) {
      task_info_->status_ = server_task_info_status::failed;
      auto l_err_str      = boost::current_exception_diagnostic_information() |
                       ranges::actions::remove_if([](const char& in_) -> bool { return in_ == '\n' || in_ == '\r'; });
      logger_->error(l_err_str);
      task_info_->last_line_log_ = l_err_str;
    }
    task_info_->end_time_ = server_task_info::zoned_time{chrono::current_zone(), std::chrono::system_clock::now()};
    logger_->flush();
    emit_signal();
  }
  void emit_signal() const {
    default_logger_raw()->warn("写出事件 {} {}", "doodle:task_info:update", (nlohmann::json{} = *task_info_).dump());
    socket_io::broadcast(socket_io::local_server_task_info_update_broadcast_t{.main_info_ = *task_info_});
  }
};

void emit_signal(const std::shared_ptr<server_task_info>& in_ptr) {
  default_logger_raw()->warn("写出事件 {} {}", "doodle:task_info:update", (nlohmann::json{} = *in_ptr).dump());
  socket_io::broadcast(socket_io::local_server_task_info_update_broadcast_t{.main_info_ = *in_ptr});
}

}  // namespace

boost::asio::awaitable<boost::beast::http::message_generator> task_instance::get(session_data_ptr in_handle) {
  auto l_list = task_info_manager().get_instance().get_all_task_info();

  co_return in_handle->make_msg(nlohmann::json{} = l_list);
}
boost::asio::awaitable<boost::beast::http::message_generator> task::get(session_data_ptr in_handle) {
  server_task_info_type l_type{server_task_info_type::unknown};
  for (auto&& i : in_handle->url_.params()) {
    if (i.has_value && i.key == "type") {
      l_type = magic_enum::enum_cast<server_task_info_type>(i.value).value_or(server_task_info_type::unknown);
    }
  }
  auto l_l    = task_info_manager().get_instance().get_all_task_info();
  auto l_list = l_l | ranges::views::filter([l_type](const server_task_info& in_info) {
                  return l_type == server_task_info_type::unknown || in_info.type_ == l_type;
                }) |
                ranges::to<std::vector>();

  for (auto&& i : l_list)
    i.get_last_line_log(
        core_set::get_set().get_cache_root() / server_task_info::logger_category / fmt::format("{}.log", i.uuid_id_)
    );
  co_return in_handle->make_msg(nlohmann::json{} = l_list);
}
void task::init_ctx() {
  static std::once_flag l_flag{};
  std::call_once(l_flag, []() {
    if (!g_ctx().contains<maya_ctx>()) g_ctx().emplace<maya_ctx>();
    if (!g_ctx().contains<ue_ctx>()) g_ctx().emplace<ue_ctx>();
    g_ctx().emplace<run_post_task_local_cancel_manager>();
    app_base::Get().on_cancel.slot().assign([](boost::asio::cancellation_type_t) {
      g_ctx().get<run_post_task_local_cancel_manager>().cancel_all();
    });
  });
}
boost::asio::awaitable<boost::beast::http::message_generator> task::post(session_data_ptr in_handle) {
  auto l_ptr  = std::make_shared<server_task_info>();
  auto l_json = in_handle->get_json();
  l_json.get_to(*l_ptr);
  l_ptr->submit_time_     = server_task_info::zoned_time{chrono::current_zone(), std::chrono::system_clock::now()};
  l_ptr->command_         = l_json["task_data"];
  l_ptr->submitter_       = boost::uuids::nil_uuid();
  l_ptr->task_id_         = boost::uuids::nil_uuid();
  l_ptr->run_computer_id_ = boost::uuids::nil_uuid();
  if (l_ptr->name_.empty()) l_ptr->name_ = fmt::to_string(l_ptr->uuid_id_);

  auto l_run_long_task_local = std::make_shared<run_long_task_local>(l_ptr);
  // 先进行数据加载, 如果出错抛出异常后直接不插入数据库
  task_info_manager().get_instance().add_task_info(l_ptr);
  l_run_long_task_local->load_form_json(l_ptr->command_);
  l_run_long_task_local->run();
  co_return in_handle->make_msg((nlohmann::json{} = *l_ptr).dump());
}

boost::asio::awaitable<boost::beast::http::message_generator> task_instance_log::get(session_data_ptr in_handle) {
  auto l_path = core_set::get_set().get_cache_root() / server_task_info::logger_category / fmt::format("{}.log", id_);
  if (!FSys::exists(l_path))
    co_return in_handle->make_error_code_msg(boost::beast::http::status::not_found, "日志不存在");
  auto l_mime = std::string{kitsu::mime_type(l_path.extension())};
  l_mime += "; charset=utf-8";
  co_return in_handle->make_msg(l_path, l_mime);
}
boost::asio::awaitable<boost::beast::http::message_generator> task_instance::delete_(session_data_ptr in_handle) {
  auto l_list = task_info_manager().get_instance().get_task_info(id_);
  if (l_list->status_ == server_task_info_status::running) {
    co_return in_handle->make_error_code_msg(
        boost::beast::http::status::method_not_allowed, "任务正在运行中, 无法删除"
    );
  }
  task_info_manager().get_instance().remove_task_info(id_);
  co_return in_handle->make_msg(nlohmann::json{});
}

boost::asio::awaitable<boost::beast::http::message_generator> task_instance::patch(session_data_ptr in_handle) {
  auto l_server_task_info = task_info_manager().get_instance().get_task_info(id_);

  server_task_info l_server_task_info_org{*l_server_task_info};
  auto l_sr   = l_server_task_info->status_;
  auto l_json = std::get<nlohmann::json>(in_handle->body_);
  if (l_json.contains("status")) l_json["status"].get_to(l_server_task_info->status_);
  if (l_json.contains("name")) l_json["name"].get_to(l_server_task_info->name_);

  if (l_sr == server_task_info_status::running && l_server_task_info->status_ == server_task_info_status::canceled)
    g_ctx().get<run_post_task_local_cancel_manager>().cancel(l_server_task_info->uuid_id_);
  co_return in_handle->make_msg(nlohmann::json{} = *l_server_task_info);
}
boost::asio::awaitable<boost::beast::http::message_generator> task_inspect_instance::post(session_data_ptr in_handle) {
  auto l_ptr   = std::make_shared<server_task_info>();
  l_ptr->type_ = server_task_info_type::check_maya;
  auto l_json  = in_handle->get_json();
  l_json.get_to(*l_ptr);
  l_ptr->submit_time_     = server_task_info::zoned_time{chrono::current_zone(), std::chrono::system_clock::now()};
  l_ptr->run_computer_id_ = boost::uuids::nil_uuid();
  l_ptr->submitter_       = boost::uuids::nil_uuid();
  l_ptr->task_id_         = boost::uuids::nil_uuid();

  auto l_arg_t            = std::make_shared<inspect_file_arg>(token_, id_);
  l_json.get_to(*l_arg_t);
  l_ptr->command_ = (nlohmann::json{} = *l_arg_t);
  task_info_manager().get_instance().add_task_info(l_ptr);

  if (l_ptr->name_.empty()) l_ptr->name_ = fmt::to_string(l_ptr->uuid_id_);
  auto l_run_long_task_local = std::make_shared<run_long_task_local>(l_ptr);
  l_run_long_task_local->set_arg(l_arg_t);
  l_run_long_task_local->run();
  co_return in_handle->make_msg((nlohmann::json{} = *l_ptr).dump());
}

boost::asio::awaitable<boost::beast::http::message_generator> task_instance_generate_uesk_file::post(
    session_data_ptr in_handle
) {
  auto l_ptr          = std::make_shared<server_task_info>();
  l_ptr->type_        = server_task_info_type::create_rig_sk;
  l_ptr->submit_time_ = server_task_info::zoned_time{chrono::current_zone(), std::chrono::system_clock::now()};
  auto l_json         = in_handle->get_json();
  l_json.get_to(*l_ptr);
  l_ptr->run_computer_id_ = boost::uuids::nil_uuid();
  l_ptr->submitter_       = boost::uuids::nil_uuid();
  l_ptr->task_id_         = boost::uuids::nil_uuid();

  auto l_client           = std::make_shared<doodle::kitsu::kitsu_client>(core_set::get_set().server_ip);
  l_client->set_token(token_);
  std::shared_ptr<export_rig_sk_arg> l_arg_t = std::make_shared<export_rig_sk_arg>();
  l_arg_t->kitsu_client_                     = l_client;
  l_arg_t->maya_file_                        = l_json["path"].get<FSys::path>();
  l_arg_t->task_id_                          = id_;
  l_ptr->command_                            = (nlohmann::json{} = *l_arg_t);
  task_info_manager().get_instance().add_task_info(l_ptr);

  if (l_ptr->name_.empty()) l_ptr->name_ = fmt::to_string(l_ptr->uuid_id_);
  auto l_run_long_task_local = std::make_shared<run_long_task_local>(l_ptr);
  l_run_long_task_local->set_arg(l_arg_t);
  l_run_long_task_local->run();
  socket_io::broadcast(socket_io::local_server_task_info_update_broadcast_t{.main_info_ = *l_ptr});
  co_return in_handle->make_msg((nlohmann::json{} = *l_ptr).dump());
}

boost::asio::awaitable<boost::beast::http::message_generator> actions_projects_shots_export_anim_fbx_local::post(
    session_data_ptr in_handle
) {
  auto l_ptr          = std::make_shared<server_task_info>();
  l_ptr->type_        = server_task_info_type::export_fbx;
  l_ptr->submit_time_ = server_task_info::zoned_time{chrono::current_zone(), std::chrono::system_clock::now()};

  auto l_json         = in_handle->get_json();
  l_json.get_to(*l_ptr);
  l_ptr->run_computer_id_ = boost::uuids::nil_uuid();
  l_ptr->submitter_       = boost::uuids::nil_uuid();
  l_ptr->task_id_         = boost::uuids::nil_uuid();

  auto l_client           = std::make_shared<doodle::kitsu::kitsu_client>(core_set::get_set().server_ip);
  l_client->set_token(token_);
  std::shared_ptr<export_fbx_arg> l_arg_t = std::make_shared<export_fbx_arg>();
  l_arg_t->kitsu_client_                  = l_client;
  l_arg_t->task_id_                       = id_;
  l_json.get_to(*l_arg_t);
  l_ptr->command_ = (nlohmann::json{} = *l_arg_t);
  task_info_manager().get_instance().add_task_info(l_ptr);

  if (l_ptr->name_.empty()) l_ptr->name_ = fmt::to_string(l_ptr->uuid_id_);
  auto l_run_long_task_local = std::make_shared<run_long_task_local>(l_ptr);
  l_run_long_task_local->set_arg(l_arg_t);
  l_run_long_task_local->run();
  socket_io::broadcast(socket_io::local_server_task_info_update_broadcast_t{.main_info_ = *l_ptr});
  co_return in_handle->make_msg((nlohmann::json{} = *l_ptr).dump());
}

boost::asio::awaitable<boost::beast::http::message_generator> actions_projects_shots_update_sim_abc_local::post(
    session_data_ptr in_handle
) {
  auto l_ptr          = std::make_shared<server_task_info>();
  l_ptr->type_        = server_task_info_type::export_sim;
  l_ptr->submit_time_ = server_task_info::zoned_time{chrono::current_zone(), std::chrono::system_clock::now()};

  auto l_json         = in_handle->get_json();
  l_json.get_to(*l_ptr);
  l_ptr->run_computer_id_ = boost::uuids::nil_uuid();
  l_ptr->submitter_       = boost::uuids::nil_uuid();
  l_ptr->task_id_         = boost::uuids::nil_uuid();

  auto l_client           = std::make_shared<doodle::kitsu::kitsu_client>(core_set::get_set().server_ip);
  l_client->set_token(token_);
  std::shared_ptr<qcloth_update_arg> l_arg_t = std::make_shared<qcloth_update_arg>();
  l_arg_t->kitsu_client_                     = l_client;
  l_arg_t->task_id_                          = id_;
  l_json.get_to(*l_arg_t);
  l_ptr->command_ = (nlohmann::json{} = *l_arg_t);
  task_info_manager().get_instance().add_task_info(l_ptr);

  if (l_ptr->name_.empty()) l_ptr->name_ = fmt::to_string(l_ptr->uuid_id_);
  auto l_run_long_task_local = std::make_shared<run_long_task_local>(l_ptr);
  l_run_long_task_local->set_arg(l_arg_t);
  l_run_long_task_local->run();
  socket_io::broadcast(socket_io::local_server_task_info_update_broadcast_t{.main_info_ = *l_ptr});
  co_return in_handle->make_msg((nlohmann::json{} = *l_ptr).dump());
}

boost::asio::awaitable<boost::beast::http::message_generator> actions_project_sync_local::post(
    session_data_ptr in_handle
) {
  auto l_ptr          = std::make_shared<server_task_info>();
  l_ptr->type_        = server_task_info_type::project_sync;
  l_ptr->submit_time_ = server_task_info::zoned_time{chrono::current_zone(), std::chrono::system_clock::now()};

  auto l_json         = in_handle->get_json();
  l_json.get_to(*l_ptr);
  l_ptr->run_computer_id_ = boost::uuids::nil_uuid();
  l_ptr->submitter_       = boost::uuids::nil_uuid();
  l_ptr->task_id_         = boost::uuids::nil_uuid();

  auto l_client           = std::make_shared<doodle::kitsu::kitsu_client>(core_set::get_set().server_ip);
  l_client->set_token(token_);

  std::shared_ptr<task_sync> l_arg_t = std::make_shared<task_sync>();
  l_arg_t->kitsu_client_             = l_client;
  l_json.get_to(*l_arg_t);
  l_ptr->command_ = (nlohmann::json{} = *l_arg_t);
  task_info_manager().get_instance().add_task_info(l_ptr);

  if (l_ptr->name_.empty()) l_ptr->name_ = fmt::to_string(l_ptr->uuid_id_);
  auto l_run_long_task_local = std::make_shared<run_long_task_local>(l_ptr);
  l_run_long_task_local->set_arg(l_arg_t);
  l_run_long_task_local->run();
  socket_io::broadcast(socket_io::local_server_task_info_update_broadcast_t{.main_info_ = *l_ptr});
  co_return in_handle->make_msg((nlohmann::json{} = *l_ptr).dump());
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(actions_local_task_update_ue_files, post) {
  auto l_ptr          = std::make_shared<server_task_info>();
  l_ptr->type_        = server_task_info_type::project_sync;
  l_ptr->submit_time_ = server_task_info::zoned_time{chrono::current_zone(), std::chrono::system_clock::now()};

  auto l_json         = in_handle->get_json();
  l_json.get_to(*l_ptr);
  l_ptr->run_computer_id_ = boost::uuids::nil_uuid();
  l_ptr->submitter_       = boost::uuids::nil_uuid();
  l_ptr->task_id_         = boost::uuids::nil_uuid();

  auto l_client           = std::make_shared<doodle::kitsu::kitsu_client>(core_set::get_set().server_ip);
  l_client->set_token(token_);

  std::shared_ptr<update_ue_files> l_arg_t = std::make_shared<update_ue_files>();
  l_arg_t->kitsu_client_                   = l_client;
  l_arg_t->task_id_                        = id_;
  l_json.get_to(*l_arg_t);
  task_info_manager().get_instance().add_task_info(l_ptr);

  if (l_ptr->name_.empty()) l_ptr->name_ = fmt::to_string(l_ptr->uuid_id_);
  auto l_run_long_task_local = std::make_shared<run_long_task_local>(l_ptr);
  l_run_long_task_local->set_arg(l_arg_t);
  l_run_long_task_local->run();
  socket_io::broadcast(socket_io::local_server_task_info_update_broadcast_t{.main_info_ = *l_ptr});
  co_return in_handle->make_msg(nlohmann::json{} = *l_ptr);
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(actions_local_task_update_movie_files, post) {
  auto l_ptr          = std::make_shared<server_task_info>();
  l_ptr->type_        = server_task_info_type::project_sync;
  l_ptr->submit_time_ = server_task_info::zoned_time{chrono::current_zone(), std::chrono::system_clock::now()};

  auto l_json         = in_handle->get_json();
  l_json.get_to(*l_ptr);
  l_ptr->run_computer_id_ = boost::uuids::nil_uuid();
  l_ptr->submitter_       = boost::uuids::nil_uuid();
  l_ptr->task_id_         = boost::uuids::nil_uuid();

  auto l_client           = std::make_shared<doodle::kitsu::kitsu_client>(core_set::get_set().server_ip);
  l_client->set_token(token_);

  std::shared_ptr<update_movie_files> l_arg_t = std::make_shared<update_movie_files>();
  l_arg_t->kitsu_client_                      = l_client;
  l_arg_t->task_id_                           = id_;
  l_json.get_to(*l_arg_t);
  task_info_manager().get_instance().add_task_info(l_ptr);

  if (l_ptr->name_.empty()) l_ptr->name_ = fmt::to_string(l_ptr->uuid_id_);
  auto l_run_long_task_local = std::make_shared<run_long_task_local>(l_ptr);
  l_run_long_task_local->set_arg(l_arg_t);
  l_run_long_task_local->run();
  socket_io::broadcast(socket_io::local_server_task_info_update_broadcast_t{.main_info_ = *l_ptr});
  co_return in_handle->make_msg(nlohmann::json{} = *l_ptr);
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(actions_local_task_update_movie_compose, post) {
  auto l_ptr          = std::make_shared<server_task_info>();
  l_ptr->type_        = server_task_info_type::project_sync;
  l_ptr->submit_time_ = server_task_info::zoned_time{chrono::current_zone(), std::chrono::system_clock::now()};

  auto l_json         = in_handle->get_json();
  l_json.get_to(*l_ptr);
  l_ptr->run_computer_id_ = boost::uuids::nil_uuid();
  l_ptr->submitter_       = boost::uuids::nil_uuid();
  l_ptr->task_id_         = boost::uuids::nil_uuid();

  auto l_client           = std::make_shared<doodle::kitsu::kitsu_client>(core_set::get_set().server_ip);
  l_client->set_token(token_);

  std::shared_ptr<update_movie_compose_files> l_arg_t = std::make_shared<update_movie_compose_files>();
  l_arg_t->kitsu_client_                              = l_client;
  l_arg_t->task_id_                                   = id_;
  l_json.get_to(*l_arg_t);
  task_info_manager().get_instance().add_task_info(l_ptr);

  if (l_ptr->name_.empty()) l_ptr->name_ = fmt::to_string(l_ptr->uuid_id_);
  auto l_run_long_task_local = std::make_shared<run_long_task_local>(l_ptr);
  l_run_long_task_local->set_arg(l_arg_t);
  l_run_long_task_local->run();
  socket_io::broadcast(socket_io::local_server_task_info_update_broadcast_t{.main_info_ = *l_ptr});
  co_return in_handle->make_msg(nlohmann::json{} = *l_ptr);
}
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(actions_local_watch_file_maya_anim, post) {
  DOODLE_TO_EXECUTOR(strand_);
  if (!folder_watcher_) folder_watcher_ = std::make_shared<doodle::exe_warp::folder_watcher_anim_fbx>();
  auto l_json     = in_handle->get_json();
  auto l_task_ids = l_json["task_ids"].get<std::vector<uuid>>();

  auto l_client   = std::make_shared<doodle::kitsu::kitsu_client>(core_set::get_set().server_ip);
  l_client->set_token(token_);


  DOODLE_TO_SELF();
}
}  // namespace doodle::http::local

namespace doodle::http {

void epiboly_actions_projects_export_anim_fbx::init_ctx() {
  static std::once_flag l_flag{};
  std::call_once(l_flag, []() {
    if (!g_ctx().contains<maya_ctx>()) g_ctx().emplace<maya_ctx>();
    if (!g_ctx().contains<ue_ctx>()) g_ctx().emplace<ue_ctx>();
    g_ctx().emplace<local::run_post_task_local_cancel_manager>();
    app_base::Get().on_cancel.slot().assign([](boost::asio::cancellation_type_t) {
      g_ctx().get<local::run_post_task_local_cancel_manager>().cancel_all();
    });
  });
}

boost::asio::awaitable<boost::beast::http::message_generator> epiboly_actions_projects_export_anim_fbx::post(
    session_data_ptr in_handle
) {
  auto l_ptr          = std::make_shared<server_task_info>();
  l_ptr->type_        = server_task_info_type::export_fbx;
  l_ptr->submit_time_ = server_task_info::zoned_time{chrono::current_zone(), std::chrono::system_clock::now()};
  // 在外包的后端中, 会载入数据库, 可以使用  project_id_ 获取项目数据
  auto l_project      = get_sqlite_database().get_by_uuid<project>(project_id_);

  auto l_json         = in_handle->get_json();
  l_json.get_to(*l_ptr);
  l_ptr->run_computer_id_                         = boost::uuids::nil_uuid();
  l_ptr->submitter_                               = boost::uuids::nil_uuid();

  std::shared_ptr<export_fbx_arg_epiboly> l_arg_t = std::make_shared<export_fbx_arg_epiboly>();
  l_json.get_to(*l_arg_t);

  l_arg_t->create_play_blast_ = true;
  l_arg_t->film_aperture_     = l_project.get_film_aperture();
  l_arg_t->size_              = l_project.get_resolution();
  l_ptr->command_             = (nlohmann::json{} = *l_arg_t);

  if (l_ptr->name_.empty()) l_ptr->name_ = fmt::to_string(l_ptr->uuid_id_);
  auto l_run_long_task_local = std::make_shared<local::run_long_task_local>(l_ptr);
  l_run_long_task_local->set_arg(l_arg_t);
  l_run_long_task_local->run();
  socket_io::broadcast(socket_io::local_server_task_info_update_broadcast_t{.main_info_ = *l_ptr});
  co_return in_handle->make_msg(nlohmann::json{} = *l_ptr);
}

}  // namespace doodle::http
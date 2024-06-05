#include "computing_time.h"

#include "doodle_core/metadata/time_point_wrap.h"
#include "doodle_core/platform/win/register_file_type.h"

#include "doodle_lib/core/http/http_session_data.h"
#include "doodle_lib/core/http/http_websocket_data.h"

//
#include <doodle_core/metadata/user.h>
#include <doodle_core/metadata/work_xlsx_task_info.h>
#include <doodle_core/time_tool/work_clock.h>

#include <doodle_lib/core/holidaycn_time.h>
#include <doodle_lib/http_client/kitsu_client.h>
namespace doodle::http {

struct computing_time_post_req_data {
  struct task_data {
    boost::uuids::uuid task_id;
    std::chrono::system_clock::time_point start_time;
    std::chrono::system_clock::time_point end_time;
    // form json
    friend void from_json(const nlohmann::json& j, task_data& p) {
      p.task_id         = boost::lexical_cast<boost::uuids::uuid>(j.at("task_id").get<std::string>());
      auto l_start_time = parse_8601<std::chrono::system_clock::time_point>(j.at("start_date").get<std::string>());
      auto l_end_time   = parse_8601<std::chrono::system_clock::time_point>(j.at("end_date").get<std::string>());
      p.start_time      = std::max(l_start_time, l_end_time);
      p.end_time        = std::min(l_start_time, l_end_time);
    }
  };
  chrono::year_month year_month_;
  boost::uuids::uuid user_id;
  std::vector<task_data> data;
  // form json
  friend void from_json(const nlohmann::json& j, computing_time_post_req_data& p) {
    std::istringstream l_year_month_stream(j.at("year_month").get<std::string>());
    l_year_month_stream >> chrono::parse("%Y-%m", p.year_month_);
    if (!l_year_month_stream) {
      throw nlohmann::json::parse_error::create(101, 0, "year_month 格式错误不是时间格式", &j);
    }
    p.user_id = boost::lexical_cast<boost::uuids::uuid>(j.at("user_id").get<std::string>());
    p.data    = j.at("data").get<std::vector<task_data>>();
  }
};

class computing_time : public std::enable_shared_from_this<computing_time> {
  user user_;
  entt::entity user_entity_{entt::null};

  work_xlsx_task_info_block block_;
  entt::entity block_entity_{entt::null};

  business::rules rules_;
  business::work_clock2 time_clock_{};
  http_session_data_ptr session_data_;

  std::shared_ptr<computing_time_post_req_data> data_;

  void find_user() {
    auto l_logger = session_data_->logger_;
    auto l_user   = std::as_const(*g_reg()).view<const user>();
    for (auto&& [e, l_u] : l_user.each()) {
      if (l_u.id_ == data_->user_id) {
        user_        = l_u;
        user_entity_ = e;
        break;
      }
    }
    if (user_.id_ == boost::uuids::nil_uuid() || user_.mobile_.empty()) {
      user_.id_           = data_->user_id;
      auto l_kitsu_client = g_ctx().get<kitsu::kitsu_client_ptr>();
      l_kitsu_client->get_user(
          user_.id_,
          boost::asio::bind_executor(
              g_io_context(), boost::beast::bind_front_handler(&computing_time::do_feach_mobile, shared_from_this())
          )
      );
    }
  }
  void do_feach_mobile(boost::system::error_code ec, nlohmann::json l_json) {
    auto l_logger = session_data_->logger_;
    if (ec) {
      l_logger->log(log_loc(), level::err, "get user failed: {}", ec.message());
      session_data_->seed_error(boost::beast::http::status::internal_server_error, ec);
      return;
    }
    try {
      user_.mobile_ = l_json["phone"].get<std::string>();
    } catch (const nlohmann::json::exception& e) {
      l_logger->log(
          log_loc(), level::err, "user {} json parse error: {}", l_json["email"].get<std::string>(), e.what()
      );
      session_data_->seed_error(
          boost::beast::http::status::internal_server_error, ec,
          fmt::format("{} {}", l_json["email"].get<std::string>(), e.what())
      );
      return;
    } catch (const std::exception& e) {
      l_logger->log(
          log_loc(), level::err, "user {} json parse error: {}", l_json["email"].get<std::string>(), e.what()
      );
      session_data_->seed_error(
          boost::beast::http::status::internal_server_error, ec,
          fmt::format("{} {}", l_json["email"].get<std::string>(), e.what())
      );
      return;
    }
    if (user_.mobile_.empty()) {
      l_logger->log(log_loc(), level::err, "user {} mobile is empty", l_json["email"].get<std::string>());
      session_data_->seed_error(
          boost::beast::http::status::internal_server_error, ec,
          fmt::format("{} mobile is empty", l_json["email"].get<std::string>())
      );
      return;
    }

    entt::handle l_handle{};
    // 创建用户
    if (user_entity_ == entt::null)
      l_handle = {*g_reg(), g_reg()->create()};
    else  // 存在用户则修改
      l_handle = {*g_reg(), user_entity_};
    l_handle.emplace_or_replace<user>(user_);
    user_entity_ = l_handle.entity();
    run_2();
  }

  void find_block() {
    auto l_block = std::as_const(*g_reg()).view<const work_xlsx_task_info_block>();
    for (auto&& [e, l_b] : l_block.each()) {
      if (l_b.year_month_ == data_->year_month_ && l_b.user_refs_ == user_entity_) {
        block_        = l_b;
        block_entity_ = e;
        break;
      }
    }
  }

  void create_time_clock() {
    chrono::sys_days l_begin_time{chrono::sys_days{data_->year_month_ / chrono::day{1}}},
        l_end_time{chrono::sys_days{data_->year_month_ / chrono::last} + chrono::days{3}};

    for (auto l_begin = l_begin_time; l_begin <= l_end_time; l_begin += chrono::days{1}) {
      // 加入工作日规定时间
      if (rules_.is_work_day(chrono::weekday{l_begin})) {
        for (auto&& l_work_time : rules_.work_pair_p) {
          time_clock_ += std::make_tuple(l_begin + l_work_time.first, l_begin + l_work_time.second);
        }
      }
    }
    // 调整节假日
    holidaycn_time2{rules_.work_pair_p}.set_clock(time_clock_);
    // 排除绝对时间
    for (auto l_begin = l_begin_time; l_begin <= l_end_time; l_begin += chrono::days{1}) {
      for (auto&& l_deduction : rules_.absolute_deduction[chrono::weekday{l_begin}.c_encoding()]) {
        time_clock_ -= std::make_tuple(l_begin + l_deduction.first, l_begin + l_deduction.second);
      }
    }
    time_clock_.cut_interval(l_begin_time, l_end_time);
  }

  // 计算时间
  void computing_time_run() {
    auto l_all_works = time_clock_(
        chrono::sys_days{data_->year_month_ / chrono::day{1}}, chrono::sys_days{data_->year_month_ / chrono::last}
    );

    work_xlsx_task_info_block l_block{
        .id_         = block_.id_.is_nil() ? core_set::get_set().get_uuid() : block_.id_,
        // .task_info_  = std::vector<work_xlsx_task_info>{},
        .year_month_ = data_->year_month_,
        .user_refs_  = user_entity_,
        .duration_   = l_all_works
    };

    // 进行排序
    std::sort(data_->data.begin(), data_->data.end(), [](const auto& l_left, const auto& l_right) {
      return l_left.start_time < l_right.start_time;
    });

    {  // 计算时间比例
      std::vector<std::double_t> l_woeks1{};
      for (auto&& l_task : data_->data) {
        l_woeks1.push_back(chrono::floor<chrono::days>(l_task.start_time - l_task.end_time).count() + 1);
      }
      auto l_works_accumulate = ranges::accumulate(l_woeks1, std::double_t{});
      chrono::sys_time_pos l_begin_time{chrono::sys_days{data_->year_month_ / chrono::day{1}}};
      for (auto i = 0; i < data_->data.size(); ++i) {
        auto l_end = time_clock_.next_time(
            l_begin_time, chrono::sys_time_pos::duration{boost::numeric_cast<std::int64_t>(
                              l_all_works.count() * (l_woeks1[i] / l_works_accumulate)
                          )}
        );
        auto l_info          = time_clock_.get_time_info(l_begin_time, l_end);
        std::string l_remark = fmt::format("{}", fmt::join(l_info, ", "));

        l_block.task_info_.emplace_back(work_xlsx_task_info{
            .id_                = core_set::get_set().get_uuid(),
            .start_time_        = l_begin_time,
            .end_time_          = l_end,
            .duration_          = chrono::duration_cast<chrono::microseconds>(l_end - l_begin_time),
            .remark_            = l_remark,
            .kitsu_task_ref_id_ = data_->data[i].task_id
        });
        l_begin_time = l_end;
      }
    }
    block_ = l_block;
    boost::asio::post(
        g_io_context(), boost::beast::bind_front_handler(&computing_time::create_block, shared_from_this())
    );
  }

  void create_block() {
    if (block_entity_ == entt::null) {
      block_entity_ = g_reg()->create();
      g_reg()->emplace<work_xlsx_task_info_block>(block_entity_, block_);
    } else {
      g_reg()->replace<work_xlsx_task_info_block>(block_entity_, block_);
    }
  }

  void run_2() {
    find_block();
    create_time_clock();
    computing_time_run();
    nlohmann::json l_json;
    l_json      = block_.task_info_;

    auto& l_req = session_data_->get_msg_body_parser<boost::beast::http::string_body>()->request_parser_->get();
    boost::beast::http::response<boost::beast::http::string_body> l_response{
        boost::beast::http::status::ok, l_req.version()
    };
    l_response.set(boost::beast::http::field::content_type, "application/json");
    l_response.body() = l_json.dump();
    l_response.keep_alive(l_req.keep_alive());
    l_response.prepare_payload();
    session_data_->seed(std::move(l_response));
  }

 public:
  void run(computing_time_post_req_data& in_data, const http_session_data_ptr& in_session_data) {
    data_         = std::make_shared<computing_time_post_req_data>(std::move(in_data));
    session_data_ = in_session_data;

    rules_        = business::rules::get_default();
    find_user();
  }
};

class computing_time_post {
 public:
  computing_time_post() : executor_(g_thread().get_executor()) {}
  ~computing_time_post() = default;
  using executor_type    = boost::asio::any_io_executor;
  boost::asio::any_io_executor executor_;
  boost::asio::any_io_executor get_executor() const { return executor_; }
  void operator()(boost::system::error_code in_error_code, const http_session_data_ptr& in_handle) const {
    auto l_logger = in_handle->logger_;
    if (in_error_code) {
      l_logger->log(log_loc(), level::err, "error: {}", in_error_code.message());
      in_handle->seed_error(boost::beast::http::status::internal_server_error, in_error_code);
      return;
    }
    auto l_req = in_handle->get_msg_body_parser<boost::beast::http::string_body>();
    auto l_str = l_req->request_parser_->get().body();
    if (!nlohmann::json::accept(l_str)) {
      l_logger->log(log_loc(), level::err, "json parse error: {}", l_str);
      in_error_code = boost::system::error_code{boost::system::errc::bad_message, boost::system::generic_category()};
      in_handle->seed_error(boost::beast::http::status::bad_request, in_error_code, "不是json字符串");
      return;
    }
    auto l_json = nlohmann::json::parse(l_str);

    computing_time_post_req_data l_data{};
    try {
      l_data = l_json.get<computing_time_post_req_data>();
    } catch (const nlohmann::json::exception& e) {
      l_logger->log(log_loc(), level::err, "json parse error: {}", e.what());
      in_error_code = boost::system::error_code{boost::system::errc::bad_message, boost::system::generic_category()};
      in_handle->seed_error(boost::beast::http::status::bad_request, in_error_code, e.what());
      return;
    } catch (const std::exception& e) {
      l_logger->log(log_loc(), level::err, "json parse error: {}", e.what());
      in_error_code = boost::system::error_code{boost::system::errc::bad_message, boost::system::generic_category()};
      in_handle->seed_error(boost::beast::http::status::bad_request, in_error_code, e.what());
      return;
    }

    auto l_computing_time = std::make_shared<computing_time>();
    l_computing_time->run(l_data, in_handle);
  }
};

class computing_time_get {
 public:
  computing_time_get() : executor_(g_io_context().get_executor()) {}
  ~computing_time_get() = default;
  using executor_type   = boost::asio::any_io_executor;
  boost::asio::any_io_executor executor_;
  boost::asio::any_io_executor get_executor() const { return executor_; }
  void operator()(boost::system::error_code in_error_code, const http_session_data_ptr& in_handle) const {}
};

void reg_computing_time(http_route& in_route) {
  in_route.reg(std::make_shared<http_function>(
      boost::beast::http::verb::post, "api/doodle/computing_time",
      session::make_http_reg_fun<boost::beast::http::string_body>(computing_time_post{})
  ));
}
}  // namespace doodle::http
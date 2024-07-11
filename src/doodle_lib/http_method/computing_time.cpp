#include "computing_time.h"

#include "doodle_core/metadata/time_point_wrap.h"
#include "doodle_core/platform/win/register_file_type.h"
#include <doodle_core/metadata/attendance.h>

#include "doodle_lib/core/http/http_session_data.h"
#include "doodle_lib/core/http/http_websocket_data.h"

#include <boost/rational.hpp>
//
#include <doodle_core/metadata/user.h>
#include <doodle_core/metadata/work_xlsx_task_info.h>
#include <doodle_core/time_tool/work_clock.h>

#include <doodle_lib/core/holidaycn_time.h>
#include <doodle_lib/http_client/kitsu_client.h>
namespace doodle::http {

class computing_time_post_impl : public std::enable_shared_from_this<computing_time_post_impl> {
 public:
  struct computing_time_post_req_data {
    struct task_data {
      boost::uuids::uuid task_id;
      chrono::local_time_pos start_time;
      chrono::local_time_pos end_time;
      // form json
      friend void from_json(const nlohmann::json& j, task_data& p) {
        p.task_id         = boost::lexical_cast<boost::uuids::uuid>(j.at("task_id").get<std::string>());
        auto l_start_time = parse_8601<chrono::local_time_pos>(j.at("start_date").get<std::string>());
        auto l_end_time   = parse_8601<chrono::local_time_pos>(j.at("end_date").get<std::string>());
        p.start_time      = std::max(l_start_time, l_end_time);
        p.end_time        = std::min(l_start_time, l_end_time);
      }
    };
    chrono::year_month year_month_;
    boost::uuids::uuid user_id;
    std::vector<task_data> data;
    // form json
    friend void from_json(const nlohmann::json& j, computing_time_post_req_data& p) {
      // std::istringstream l_year_month_stream(j.at("year_month").get<std::string>());
      // l_year_month_stream >> chrono::parse("%Y-%m", p.year_month_);
      // if (!l_year_month_stream) {
      //   throw nlohmann::json::parse_error::create(101, 0, "year_month 格式错误不是时间格式", &j);
      // }
      // p.user_id = boost::lexical_cast<boost::uuids::uuid>(j.at("user_id").get<std::string>());
      p.data = j.at("data").get<std::vector<task_data>>();
    }
  };

 private:
  user user_;
  entt::entity user_entity_{entt::null};

  work_xlsx_task_info_block block_{.id_ = boost::uuids::nil_uuid()};
  entt::entity block_entity_{entt::null};

  business::rules rules_;
  business::work_clock2 time_clock_{};
  http_session_data_ptr session_data_;
  //  共用
  chrono::year_month year_month_{};

  // post 使用
  std::shared_ptr<computing_time_post_req_data> data_;

  // patch 使用
  boost::uuids::uuid user_id_{}, task_id_{};
  chrono::microseconds duration_{};

  // void find_user() {
  //   auto l_logger = session_data_->logger_;
  //   auto l_user   = std::as_const(*g_reg()).view<const user>();
  //   for (auto&& [e, l_u] : l_user.each()) {
  //     if (l_u.id_ == data_->user_id) {
  //       user_        = l_u;
  //       user_entity_ = e;
  //       break;
  //     }
  //   }
  //   if (user_entity_ == entt::null) {
  //     l_logger->log(log_loc(), level::err, "user {} not found", data_->user_id);
  //     session_data_->seed_error(
  //         boost::beast::http::status::not_found, boost::system::errc::make_error_code(boost::system::errc::no_link),
  //         fmt::format("user {} not found", data_->user_id)
  //     );
  //     return;
  //   }
  // }

  // void find_block() {
  //   auto l_block = std::as_const(*g_reg()).view<const work_xlsx_task_info_block>();
  //   for (auto&& [e, l_b] : l_block.each()) {
  //     if (l_b.year_month_ == year_month_ && l_b.user_refs_ == user_entity_) {
  //       block_        = l_b;
  //       block_entity_ = e;
  //       break;
  //     }
  //   }
  // }

  // void find_main() {
  //   find_user();
  //   find_block();
  //   boost::asio::post(
  //       g_thread(), boost::beast::bind_front_handler(&computing_time_post_impl::compute_time_thread,
  //       shared_from_this())
  //   );
  // }

  // // 计算 时间和发送数据 其他线程
  // void compute_time_thread() {
  //   create_time_clock();
  //   computing_time_run();
  //   boost::asio::post(
  //       g_io_context(), boost::beast::bind_front_handler(&computing_time_post_impl::create_block, shared_from_this())
  //   );
  //   // boost::asio::post(g_thread(), [l_self = shared_from_this(), this]() {
  //   nlohmann::json l_json{};
  //   l_json["data"]     = block_.task_info_;
  //   l_json["id"]       = fmt::to_string(block_.id_);
  //   l_json["duration"] = block_.duration_.count();

  //   auto& l_req        =
  //   session_data_->get_msg_body_parser<boost::beast::http::string_body>()->request_parser_->get();
  //   boost::beast::http::response<boost::beast::http::string_body> l_response{
  //       boost::beast::http::status::ok, l_req.version()
  //   };
  //   l_response.set(boost::beast::http::field::content_type, "application/json");
  //   l_response.body() = l_json.dump();
  //   l_response.keep_alive(l_req.keep_alive());
  //   l_response.prepare_payload();
  //   session_data_->seed(std::move(l_response));
  // }

  // void create_time_clock() {
  //   chrono::local_days l_begin_time{chrono::local_days{year_month_ / chrono::day{1}}},
  //       l_end_time{chrono::local_days{year_month_ / chrono::last} + chrono::days{3}};

  //   for (auto l_begin = l_begin_time; l_begin <= l_end_time; l_begin += chrono::days{1}) {
  //     // 加入工作日规定时间
  //     if (rules_.is_work_day(chrono::weekday{l_begin})) {
  //       for (auto&& l_work_time : rules_.work_pair_p) {
  //         time_clock_ += std::make_tuple(l_begin + l_work_time.first, l_begin + l_work_time.second);
  //       }
  //     }
  //   }
  //   // 调整节假日
  //   holidaycn_time2{rules_.work_pair_p}.set_clock(time_clock_);

  //   // 添加审批单
  //   for (auto l_it = l_begin_time; l_it <= l_end_time; l_it += chrono::days{1}) {
  //     if (user_.attendance_block_.contains(chrono::year_month_day{l_it})) {
  //       auto l_block =
  //           std::as_const(*g_reg()).get<const
  //           attendance_block>(user_.attendance_block_.at(chrono::year_month_day{l_it})
  //           );
  //       for (auto&& l_att : l_block.attendance_block_) {
  //         time_clock_ += std::make_tuple(l_att.start_time_, l_att.end_time_, l_att.remark_);
  //       }
  //     }
  //   }

  //   // #ifndef NDEBUG
  //   //     auto l_logger = session_data_->logger_;
  //   //     l_logger->log(log_loc(), level::info, "work_pair_p: {}", fmt::join(rules_.work_pair_p, ", "));
  //   //     l_logger->log(log_loc(), level::info, "work: {}", time_clock_.debug_print());
  //   // #endif

  //   // 排除绝对时间
  //   for (auto l_begin = l_begin_time; l_begin <= l_end_time; l_begin += chrono::days{1}) {
  //     for (auto&& l_deduction : rules_.absolute_deduction[chrono::weekday{l_begin}.c_encoding()]) {
  //       time_clock_ -= std::make_tuple(l_begin + l_deduction.first, l_begin + l_deduction.second);
  //     }
  //   }
  //   time_clock_.cut_interval(l_begin_time, l_end_time);
  // }

  // // 计算时间
  // void computing_time_run() {
  //   auto l_end_time  = chrono::local_days{(year_month_ + chrono::months{1}) / chrono::day{1}} - chrono::seconds{1};
  //   auto l_all_works = time_clock_(chrono::local_days{year_month_ / chrono::day{1}}, l_end_time);

  //   // #ifndef NDEBUG
  //   //     auto l_logger = session_data_->logger_;
  //   //     l_logger->log(
  //   //         log_loc(), level::info, "begin_time: {}, end_time: {} work_time: {}",
  //   //         chrono::local_days{year_month_ / chrono::day{1}}, l_end_time, l_all_works
  //   //     );
  //   // #endif

  //   work_xlsx_task_info_block l_block{
  //       .id_         = block_.id_.is_nil() ? core_set::get_set().get_uuid() : block_.id_,
  //       // .task_info_  = std::vector<work_xlsx_task_info>{},
  //       .year_month_ = year_month_,
  //       .user_refs_  = user_entity_,
  //       .duration_   = l_all_works
  //   };

  //   // 进行排序
  //   std::sort(data_->data.begin(), data_->data.end(), [](const auto& l_left, const auto& l_right) {
  //     return l_left.start_time < l_right.start_time;
  //   });

  //   {  // 计算时间比例
  //     std::vector<std::int64_t> l_woeks1{};
  //     for (auto&& l_task : data_->data) {
  //       l_woeks1.push_back(chrono::floor<chrono::days>(l_task.start_time - l_task.end_time).count() + 1);
  //     }
  //     auto l_works_accumulate = ranges::accumulate(l_woeks1, std::int64_t{});
  //     std::vector<chrono::seconds> l_woeks2{};
  //     using rational_int = boost::rational<std::int64_t>;
  //     for (auto i = 0; i < l_woeks1.size(); ++i) {
  //       l_woeks2.push_back(chrono::seconds{
  //           boost::rational_cast<std::int64_t>(rational_int{l_woeks1[i], l_works_accumulate} * l_all_works.count())
  //       });
  //     }

  //     // #ifndef NDEBUG
  //     //       l_logger->log(
  //     //           log_loc(), level::info, "woeks1: {}, woeks2: {}", fmt::join(l_woeks1, ", "), fmt::join(l_woeks2,
  //     ",
  //     //           ")
  //     //       );
  //     // #endif

  //     chrono::local_time_pos l_begin_time{chrono::local_days{year_month_ / chrono::day{1}}};
  //     for (auto i = 0; i < data_->data.size(); ++i) {
  //       auto l_end           = time_clock_.next_time(l_begin_time, l_woeks2[i]);

  //       // BOOST_ASSERT(time_clock_(l_begin_time, l_end) == l_woeks2[i]);
  //       // #ifndef NDEBUG
  //       //         l_logger->log(log_loc(), level::info, "woeks1: {}, woeks2: {}", time_clock_(l_begin_time, l_end),
  //       //         l_woeks2[i]);
  //       // #endif
  //       auto l_info          = time_clock_.get_time_info(l_begin_time, l_end);
  //       std::string l_remark = fmt::format("{}", fmt::join(l_info, ", "));

  //       l_block.task_info_.emplace_back(work_xlsx_task_info{
  //           .id_                = core_set::get_set().get_uuid(),
  //           .start_time_        = {chrono::current_zone(), l_begin_time},
  //           .end_time_          = {chrono::current_zone(), l_end},
  //           .duration_          = chrono::duration_cast<chrono::seconds>(l_woeks2[i]),
  //           .remark_            = l_remark,
  //           .kitsu_task_ref_id_ = data_->data[i].task_id
  //       });
  //       l_begin_time = l_end;
  //     }
  //   }
  //   block_ = l_block;
  // }

  // // 主线程
  // void create_block() {
  //   if (block_entity_ == entt::null) {
  //     block_entity_ = g_reg()->create();
  //     g_reg()->emplace<work_xlsx_task_info_block>(block_entity_, block_);
  //   } else {
  //     g_reg()->replace<work_xlsx_task_info_block>(block_entity_, block_);
  //   }

  //   g_reg()->patch<user>(user_entity_).task_block_[year_month_] = block_entity_;
  // }

  // ///////////////////////////////////////
  // // 主线程
  // void find_data() {
  //   auto l_logger = session_data_->logger_;
  //   auto l_user   = std::as_const(*g_reg()).view<const user>();
  //   for (auto&& [e, l_u] : l_user.each()) {
  //     if (l_u.id_ == user_id_) {
  //       user_        = l_u;
  //       user_entity_ = e;
  //       break;
  //     }
  //   }
  //   if (user_entity_ == entt::null) {
  //     l_logger->log(log_loc(), level::err, "user {} not found", user_id_);
  //     session_data_->seed_error(
  //         boost::beast::http::status::not_found, boost::system::errc::make_error_code(boost::system::errc::no_link),
  //         fmt::format("user {} not found", user_id_)
  //     );
  //     return;
  //   }

  //   if (user_.task_block_.contains(year_month_)) {
  //     block_        = std::as_const(*g_reg()).get<const
  //     work_xlsx_task_info_block>(user_.task_block_.at(year_month_)); block_entity_ =
  //     user_.task_block_.at(year_month_);
  //   } else {
  //     auto l_year_month_str =
  //         fmt::format("{}-{}", std::int32_t{year_month_.year()}, std::uint32_t{year_month_.month()});

  //     l_logger->log(log_loc(), level::err, "user {} year_month {} not found", user_id_, l_year_month_str);
  //     session_data_->seed_error(
  //         boost::beast::http::status::not_found, boost::system::errc::make_error_code(boost::system::errc::no_link),
  //         fmt::format("user {} year_month {} not found", user_id_, l_year_month_str)
  //     );
  //     return;
  //   }
  //   boost::asio::post(
  //       g_thread(), boost::beast::bind_front_handler(&computing_time_post_impl::patch_time, shared_from_this())
  //   );
  // }

  // void patch_time() try {
  //   create_time_clock();

  //   auto l_logger  = session_data_->logger_;
  //   auto l_block   = block_;

  //   auto l_task_it = std::find_if(l_block.task_info_.begin(), l_block.task_info_.end(), [this](const auto& l_task) {
  //     return l_task.id_ == task_id_;
  //   });
  //   if (l_task_it == l_block.task_info_.end()) {
  //     l_logger->log(log_loc(), level::err, "task {} not found", task_id_);
  //     session_data_->seed_error(
  //         boost::beast::http::status::not_found, boost::system::errc::make_error_code(boost::system::errc::no_link),
  //         fmt::format("task {} not found", task_id_)
  //     );
  //     return;
  //   }

  //   // 只有一个任务, 不可以调整
  //   if (l_block.task_info_.size() != 1) {
  //     using rational_int = boost::rational<std::int64_t>;
  //     rational_int l_duration_int{
  //         (l_task_it->duration_ - duration_).count(), boost::numeric_cast<std::int64_t>(l_block.task_info_.size() -
  //         1)
  //     };
  //     // 调整差值
  //     l_task_it->duration_ = duration_;
  //     for (auto&& l_task : l_block.task_info_) {
  //       if (l_task.id_ != task_id_) {
  //         l_task.duration_ += chrono::microseconds{boost::rational_cast<std::int64_t>(l_duration_int)};
  //       }
  //     }

  //     // 计算时间开始结束
  //     chrono::local_time_pos l_begin_time{chrono::local_days{year_month_ / chrono::day{1}}};
  //     for (auto i = 0; i < l_block.task_info_.size(); ++i) {
  //       auto l_end                        = time_clock_.next_time(l_begin_time, l_block.task_info_[i].duration_);
  //       l_block.task_info_[i].start_time_ = l_begin_time;
  //       l_block.task_info_[i].end_time_   = l_end;
  //       l_begin_time                      = l_end;
  //     }
  //     block_ = l_block;
  //     boost::asio::post(
  //         g_io_context(), boost::beast::bind_front_handler(&computing_time_post_impl::create_block,
  //         shared_from_this())
  //     );
  //   }

  //   nlohmann::json l_json;
  //   l_json["data"]     = block_.task_info_;
  //   l_json["id"]       = fmt::to_string(block_.id_);
  //   l_json["duration"] = block_.duration_.count();

  //   auto& l_req        =
  //   session_data_->get_msg_body_parser<boost::beast::http::string_body>()->request_parser_->get();
  //   boost::beast::http::response<boost::beast::http::string_body> l_response{
  //       boost::beast::http::status::ok, l_req.version()
  //   };
  //   l_response.set(boost::beast::http::field::content_type, "application/json");
  //   l_response.body() = l_json.dump();
  //   l_response.keep_alive(l_req.keep_alive());
  //   l_response.prepare_payload();
  //   session_data_->seed(std::move(l_response));
  // } catch (...) {
  //   session_data_->seed_error(
  //       boost::beast::http::status::internal_server_error,
  //       boost::system::errc::make_error_code(boost::system::errc::bad_message),
  //       boost::current_exception_diagnostic_information()
  //   );
  // }

 public:
  explicit computing_time_post_impl(http_session_data_ptr in_session_data)
      : session_data_(std::move(in_session_data)) {}

  // 这个不是主线程
  void run_post(computing_time_post_req_data& in_data) {
    data_       = std::make_shared<computing_time_post_req_data>(std::move(in_data));
    rules_      = business::rules::get_default();
    year_month_ = data_->year_month_;

    // boost::asio::post(
    //     g_io_context(), boost::beast::bind_front_handler(&computing_time_post_impl::find_main, shared_from_this())
    // );
  }

  void run_patch(
      boost::uuids::uuid in_user_id, boost::uuids::uuid in_task_id, chrono::year_month in_year_month,
      chrono::microseconds in_duration
  ) {
    user_id_    = in_user_id;
    task_id_    = in_task_id;
    year_month_ = in_year_month;
    duration_   = in_duration;

    rules_      = business::rules::get_default();
    // boost::asio::post(
    //     g_io_context(), boost::beast::bind_front_handler(&computing_time_post_impl::find_data, shared_from_this())
    // );
  }
};

boost::asio::awaitable<boost::beast::http::message_generator> computing_time_post(session_data_ptr in_handle) {
  auto l_logger = in_handle->logger_;

  if (in_handle->content_type_ != http::detail::content_type::application_json)
    co_return in_handle->make_error_code_msg(
        boost::beast::http::status::bad_request, boost::system::errc::make_error_code(boost::system::errc::bad_message),
        "不是json请求"
    );

  auto l_json = std::get<nlohmann::json>(in_handle->body_);

  computing_time_post_impl::computing_time_post_req_data l_data{};

  try {
    l_data         = l_json.get<computing_time_post_impl::computing_time_post_req_data>();

    l_data.user_id = boost::lexical_cast<boost::uuids::uuid>(in_handle->capture_->get("user_id"));
    std::istringstream l_year_month_stream{in_handle->capture_->get("year_month")};
    l_year_month_stream >> chrono::parse("%Y-%m", l_data.year_month_);
  } catch (const std::exception& e) {
    l_logger->log(log_loc(), level::err, "error: {}", e.what());
    co_return in_handle->make_error_code_msg(
        boost::beast::http::status::bad_request,
        boost::system::error_code{boost::system::errc::bad_message, boost::system::generic_category()}, e.what()
    );
  } catch (...) {
    l_logger->log(log_loc(), level::err, boost::current_exception_diagnostic_information());
    co_return in_handle->make_error_code_msg(
        boost::beast::http::status::internal_server_error,
        boost::system::errc::make_error_code(boost::system::errc::bad_message),
        boost::current_exception_diagnostic_information()
    );
  }

  auto l_computing_time = std::make_shared<computing_time_post_impl>(in_handle);
  l_computing_time->run_post(l_data);
}

boost::asio::awaitable<boost::beast::http::message_generator> computing_time_get(session_data_ptr in_handle) {
  auto l_logger         = in_handle->logger_;

  auto l_user_str       = in_handle->capture_->get("user_id");
  auto l_year_month_str = in_handle->capture_->get("year_month");

  boost::uuids::uuid l_user_id{};
  chrono::year_month l_year_month{};
  try {
    l_user_id = boost::lexical_cast<boost::uuids::uuid>(l_user_str);
    std::istringstream l_year_month_stream{l_year_month_str};
    l_year_month_stream >> chrono::parse("%Y-%m", l_year_month);
  } catch (const std::exception& e) {
    l_logger->log(log_loc(), level::err, "url parse error: {}", e.what());
    co_return in_handle->make_error_code_msg(
        boost::beast::http::status::bad_request,
        boost::system::error_code{boost::system::errc::bad_message, boost::system::generic_category()}, e.what()
    );
  } catch (...) {
    l_logger->log(log_loc(), level::err, boost::current_exception_diagnostic_information());
    co_return in_handle->make_error_code_msg(
        boost::beast::http::status::internal_server_error,
        boost::system::errc::make_error_code(boost::system::errc::bad_message),
        boost::current_exception_diagnostic_information()
    );
  }

  boost::beast::http::response<boost::beast::http::string_body> l_response{
      boost::beast::http::status::ok, in_handle->version_
  };
  l_response.set(boost::beast::http::field::content_type, "application/json");
  l_response.keep_alive(in_handle->keep_alive_);

  // 切换线程
  co_await boost::asio::post(boost::asio::bind_executor(g_strand(), boost::asio::use_awaitable));
  auto l_user = std::as_const(*g_reg()).view<const user>();
  for (auto&& [e, l_u] : l_user.each()) {
    if (l_u.id_ == l_user_id && l_u.task_block_.contains(l_year_month)) {
      auto& l_b = std::as_const(*g_reg()).get<const work_xlsx_task_info_block>(l_u.task_block_.at(l_year_month));
      nlohmann::json l_json{};
      l_json["data"]     = l_b.task_info_;
      l_json["id"]       = fmt::to_string(l_b.id_);
      l_json["duration"] = l_b.duration_.count();
      l_response.result(boost::beast::http::status::ok);
      l_response.body() = l_json.dump();
      l_response.prepare_payload();
      co_return std::move(l_response);
    }
  }
  l_response.result(boost::beast::http::status::not_found);
  l_response.body() = "";
  l_response.prepare_payload();
  co_return std::move(l_response);
}

boost::asio::awaitable<boost::beast::http::message_generator> computing_time_patch(session_data_ptr in_handle) {
  auto l_logger = in_handle->logger_;

  if (in_handle->content_type_ != http::detail::content_type::application_json)
    co_return in_handle->make_error_code_msg(
        boost::beast::http::status::bad_request, boost::system::errc::make_error_code(boost::system::errc::bad_message),
        "不是json请求"
    );
  auto& l_json          = std::get<nlohmann::json>(in_handle->body_);

  auto l_user_str       = in_handle->capture_->get("user_id");
  auto l_year_month_str = in_handle->capture_->get("year_month");
  auto l_task_id_str    = in_handle->capture_->get("task_id");
  boost::uuids::uuid l_user_id{}, l_task_id{};
  chrono::year_month l_year_month{};
  chrono::microseconds l_duration{};
  try {
    l_task_id = boost::lexical_cast<boost::uuids::uuid>(l_task_id_str);
    l_user_id = boost::lexical_cast<boost::uuids::uuid>(l_user_str);
    std::istringstream l_year_month_stream{l_year_month_str};
    l_year_month_stream >> chrono::parse("%Y-%m", l_year_month);
    l_duration = chrono::microseconds{l_json["duration"].get<std::int64_t>()};
  } catch (const std::exception& e) {
    l_logger->log(log_loc(), level::err, "url parse error: {}", e.what());
    co_return in_handle->make_error_code_msg(
        boost::beast::http::status::bad_request,
        boost::system::error_code{boost::system::errc::bad_message, boost::system::generic_category()}, e.what()
    );
  } catch (...) {
    l_logger->log(log_loc(), level::err, boost::current_exception_diagnostic_information());
    co_return in_handle->make_error_code_msg(
        boost::beast::http::status::internal_server_error,
        boost::system::errc::make_error_code(boost::system::errc::bad_message),
        boost::current_exception_diagnostic_information()
    );
  }

  auto l_computing_time = std::make_shared<computing_time_post_impl>(in_handle);
  l_computing_time->run_patch(l_user_id, l_task_id, l_year_month, l_duration);
}

boost::asio::awaitable<boost::beast::http::message_generator> computing_time_patch_delete(session_data_ptr in_handle) {
  auto l_logger                = in_handle->logger_;

  auto l_computing_time_id_str = in_handle->capture_->get("computing_time_id");
  boost::uuids::uuid l_computing_time_id{};
  try {
    l_computing_time_id = boost::lexical_cast<boost::uuids::uuid>(l_computing_time_id_str);
  } catch (const std::exception& e) {
    l_logger->log(log_loc(), level::err, "url parse error: {}", e.what());
    co_return in_handle->make_error_code_msg(
        boost::beast::http::status::bad_request,
        boost::system::error_code{boost::system::errc::bad_message, boost::system::generic_category()}, e.what()
    );
  } catch (...) {
    l_logger->log(log_loc(), level::err, boost::current_exception_diagnostic_information());
    co_return in_handle->make_error_code_msg(
        boost::beast::http::status::internal_server_error,
        boost::system::errc::make_error_code(boost::system::errc::bad_message),
        boost::current_exception_diagnostic_information()
    );
  }
  auto l_v = std::as_const(*g_reg()).view<const work_xlsx_task_info_block>();
  for (auto&& [e, l_block] : l_v.each()) {
    if (auto l_it = std::ranges::find_if(
            l_block.task_info_, [l_computing_time_id](const auto& l_task) { return l_task.id_ == l_computing_time_id; }
        );
        l_it != l_block.task_info_.end()) {
      auto l_user_e = l_block.user_refs_;
      auto l_y_m    = l_block.year_month_;
      co_await boost::asio::post(boost::asio::bind_executor(g_strand(), boost::asio::use_awaitable));
      if (g_reg()->valid(e)) {
        std::erase_if(
            g_reg()->patch<work_xlsx_task_info_block>(e).task_info_,
            [l_computing_time_id](const auto& l_task) { return l_task.id_ == l_computing_time_id; }
        );
      }
      boost::beast::http::response<boost::beast::http::empty_body> l_response{
          boost::beast::http::status::ok, in_handle->version_
      };
      l_response.keep_alive(in_handle->keep_alive_);
      l_response.prepare_payload();
      co_return std::move(l_response);
    }
  }

  boost::beast::http::response<boost::beast::http::empty_body> l_response{
      boost::beast::http::status::not_found, in_handle->version_
  };
  l_response.keep_alive(in_handle->keep_alive_);
  l_response.prepare_payload();
  co_return std::move(l_response);
}

void reg_computing_time(http_route& in_route) {
  in_route
      .reg(std::make_shared<http_function>(
          boost::beast::http::verb::post, "api/doodle/computing_time/{user_id}/{year_month}", computing_time_post
      ))
      .reg(std::make_shared<http_function>(
          boost::beast::http::verb::get, "api/doodle/computing_time/{user_id}/{year_month}", computing_time_get
      ))
      .reg(std::make_shared<http_function>(
          boost::beast::http::verb::patch, "api/doodle/computing_time/{user_id}/{year_month}/{task_id}",
          computing_time_patch
      ))
      .reg(std::make_shared<http_function>(
          boost::beast::http::verb::delete_, "api/doodle/computing_time/{computing_time_id}",
          computing_time_patch_delete
      ))

      ;
}
}  // namespace doodle::http
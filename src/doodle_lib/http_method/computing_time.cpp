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
#include <doodle_lib/http_method/share_fun.h>

namespace doodle::http {
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


std::tuple<entt::handle, work_xlsx_task_info_block> find_block_handle(
  entt::registry& reg, const chrono::year_month& in_year_month, const entt::entity& in_user
) {
  auto l_v = reg.view<const work_xlsx_task_info_block>();
  for (auto&& [e, l_block] : l_v.each()) {
    if (l_block.year_month_ == in_year_month && l_block.user_refs_ == in_user) return {{reg, e}, l_block};
  }
  return {};
}

business::work_clock2 create_time_clock(const chrono::year_month& in_year_month, const user& in_user) {
  business::work_clock2 l_time_clock_{};
  auto l_rules_ = business::rules::get_default();
  chrono::local_days l_begin_time{chrono::local_days{in_year_month / chrono::day{1}}},
                     l_end_time{chrono::local_days{in_year_month / chrono::last} + chrono::days{3}};

  for (auto l_begin = l_begin_time; l_begin <= l_end_time; l_begin += chrono::days{1}) {
    // 加入工作日规定时间
    if (l_rules_.is_work_day(chrono::weekday{l_begin})) {
      for (auto&& l_work_time : l_rules_.work_pair_p) {
        l_time_clock_ += std::make_tuple(l_begin + l_work_time.first, l_begin + l_work_time.second);
      }
    }
  }
  // 调整节假日
  holidaycn_time2{l_rules_.work_pair_p}.set_clock(l_time_clock_);

  // 添加审批单
  for (auto l_it = l_begin_time; l_it <= l_end_time; l_it += chrono::days{1}) {
    if (in_user.attendance_block_.contains(chrono::year_month_day{l_it})) {
      auto l_block =
          std::as_const(*g_reg()).get<const attendance_block>(in_user.attendance_block_.at(chrono::year_month_day{l_it})
          );
      for (auto&& l_att : l_block.attendance_block_) {
        l_time_clock_ += std::make_tuple(l_att.start_time_, l_att.end_time_, l_att.remark_);
      }
    }
  }

  // #ifndef NDEBUG
  //     auto l_logger = session_data_->logger_;
  //     l_logger->log(log_loc(), level::info, "work_pair_p: {}", fmt::join(l_rules_.work_pair_p, ", "));
  //     l_logger->log(log_loc(), level::info, "work: {}", l_time_clock_.debug_print());
  // #endif

  // 排除绝对时间
  for (auto l_begin = l_begin_time; l_begin <= l_end_time; l_begin += chrono::days{1}) {
    for (auto&& l_deduction : l_rules_.absolute_deduction[chrono::weekday{l_begin}.c_encoding()]) {
      l_time_clock_ -= std::make_tuple(l_begin + l_deduction.first, l_begin + l_deduction.second);
    }
  }
  l_time_clock_.cut_interval(l_begin_time, l_end_time);
  return l_time_clock_;
}

// 计算时间
work_xlsx_task_info_block computing_time_run(
  const chrono::year_month& in_year_month, const business::work_clock2& in_time_clock, const entt::handle& in_user,
  const work_xlsx_task_info_block& in_block, computing_time_post_req_data& in_data
) {
  auto l_end_time  = chrono::local_days{(in_year_month + chrono::months{1}) / chrono::day{1}} - chrono::seconds{1};
  auto l_all_works = in_time_clock(chrono::local_days{in_year_month / chrono::day{1}}, l_end_time);

  // #ifndef NDEBUG
  //     auto l_logger = session_data_->logger_;
  //     l_logger->log(
  //         log_loc(), level::info, "begin_time: {}, end_time: {} work_time: {}",
  //         chrono::local_days{year_month_ / chrono::day{1}}, l_end_time, l_all_works
  //     );
  // #endif

  work_xlsx_task_info_block l_block{
      .id_ = in_block.id_.is_nil() ? core_set::get_set().get_uuid() : in_block.id_,
      // .task_info_  = std::vector<work_xlsx_task_info>{},
      .year_month_ = in_year_month,
      .user_refs_ = in_user,
      .duration_ = l_all_works
  };

  // 进行排序

  std::ranges::sort(in_data.data, [](auto&& l_left, auto&& l_right) { return l_left.start_time < l_right.start_time; });
  // });

  {
    // 计算时间比例
    std::vector<std::int64_t> l_woeks1{};
    for (auto&& l_task : in_data.data) {
      l_woeks1.push_back(chrono::floor<chrono::days>(l_task.start_time - l_task.end_time).count() + 1);
    }
    auto l_works_accumulate = ranges::accumulate(l_woeks1, std::int64_t{});
    std::vector<chrono::seconds> l_woeks2{};
    using rational_int = boost::rational<std::int64_t>;
    for (auto i = 0; i < l_woeks1.size(); ++i) {
      l_woeks2.push_back(chrono::seconds{
          boost::rational_cast<std::int64_t>(rational_int{l_woeks1[i], l_works_accumulate} * l_all_works.count())
      });
    }

    // #ifndef NDEBUG
    //       l_logger->log(
    //           log_loc(), level::info, "woeks1: {}, woeks2: {}", fmt::join(l_woeks1, ", "), fmt::join(l_woeks2,
    // ,
    //           ")
    //       );
    // #endif

    chrono::local_time_pos l_begin_time{chrono::local_days{in_year_month / chrono::day{1}}};
    for (auto i = 0; i < in_data.data.size(); ++i) {
      auto l_end = in_time_clock.next_time(l_begin_time, l_woeks2[i]);

      // BOOST_ASSERT(in_time_clock(l_begin_time, l_end) == l_woeks2[i]);
      // #ifndef NDEBUG
      //         l_logger->log(log_loc(), level::info, "woeks1: {}, woeks2: {}", in_time_clock(l_begin_time, l_end),
      //         l_woeks2[i]);
      // #endif
      auto l_info          = in_time_clock.get_time_info(l_begin_time, l_end);
      std::string l_remark = fmt::format("{}", fmt::join(l_info, ", "));

      l_block.task_info_.emplace_back(work_xlsx_task_info{
          .id_ = core_set::get_set().get_uuid(),
          .start_time_ = {chrono::current_zone(), l_begin_time},
          .end_time_ = {chrono::current_zone(), l_end},
          .duration_ = chrono::duration_cast<chrono::seconds>(l_woeks2[i]),
          .remark_ = l_remark,
          .kitsu_task_ref_id_ = in_data.data[i].task_id
      });
      l_begin_time = l_end;
    }
  }
  return l_block;
}

std::optional<work_xlsx_task_info_block> patch_time(
  const business::work_clock2& in_time_clock, const work_xlsx_task_info_block& in_block,
  const boost::uuids::uuid& in_task_id_, const chrono::microseconds& in_duration, const logger_ptr& in_logger_ptr
) {
  auto l_block = in_block;

  auto l_task_it = std::ranges::find_if(l_block.task_info_, [&in_task_id_](const auto& l_task) {
    return l_task.id_ == in_task_id_;
  });
  if (l_task_it == l_block.task_info_.end()) {
    in_logger_ptr->log(log_loc(), level::err, "task {} not found", in_task_id_);
    return {};
  }

  // 只有一个任务, 不可以调整
  if (l_block.task_info_.size() == 1) return l_block;

  {
    using rational_int = boost::rational<std::int64_t>;
    rational_int l_duration_int{
        (l_task_it->duration_ - in_duration).count(), boost::numeric_cast<std::int64_t>(l_block.task_info_.size() - 1)
    };
    // 调整差值
    l_task_it->duration_ = in_duration;
    for (auto&& l_task : l_block.task_info_) {
      if (l_task.id_ != in_task_id_) {
        l_task.duration_ += chrono::microseconds{boost::rational_cast<std::int64_t>(l_duration_int)};
      }
    }

    // 计算时间开始结束
    chrono::local_time_pos l_begin_time{chrono::local_days{l_block.year_month_ / chrono::day{1}}};
    for (auto i = 0; i < l_block.task_info_.size(); ++i) {
      auto l_end                        = in_time_clock.next_time(l_begin_time, l_block.task_info_[i].duration_);
      l_block.task_info_[i].start_time_ = l_begin_time;
      l_block.task_info_[i].end_time_   = l_end;
      l_begin_time                      = l_end;
    }
  }
  return l_block;
}

boost::asio::awaitable<boost::beast::http::message_generator> computing_time_post(session_data_ptr in_handle) {
  auto l_logger = in_handle->logger_;

  if (in_handle->content_type_ != http::detail::content_type::application_json)
    co_return in_handle->make_error_code_msg(
      boost::beast::http::status::bad_request, boost::system::errc::make_error_code(boost::system::errc::bad_message),
      "不是json请求"
    );

  auto l_json = std::get<nlohmann::json>(in_handle->body_);

  computing_time_post_req_data l_data{};

  try {
    l_data         = l_json.get<computing_time_post_req_data>();
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
  // 保存现在的执行线程
  auto l_this_exe = co_await boost::asio::this_coro::executor;
  // 切换到主线程
  co_await boost::asio::post(boost::asio::bind_executor(g_strand(), boost::asio::use_awaitable));
  auto [l_user_handle, l_user] = find_user_handle(*g_reg(), l_data.user_id);
  if (!l_user_handle)
    co_return in_handle->make_error_code_msg(
      boost::beast::http::status::not_found,
      boost::system::error_code{boost::system::errc::bad_message, boost::system::generic_category()}, "找不到用户"
    );
  auto [l_block_handle, l_block_old] = find_block_handle(*g_reg(), l_data.year_month_, l_user_handle);

  // 切换到自己的线程
  co_await boost::asio::post(boost::asio::bind_executor(l_this_exe, boost::asio::use_awaitable));
  auto l_time_clock = create_time_clock(l_data.year_month_, l_user);
  auto l_block      = computing_time_run(l_data.year_month_, l_time_clock, l_user_handle, l_block_old, l_data);

  // 切换到主线程
  co_await boost::asio::post(boost::asio::bind_executor(g_strand(), boost::asio::use_awaitable));
  if (!l_block_handle) {
    l_block_handle = {*g_reg(), g_reg()->create()};
    l_user_handle.emplace<work_xlsx_task_info_block>(l_block);
  } else {
    l_block_handle.emplace_or_replace<work_xlsx_task_info_block>(l_block);
  }

  // 切换到自己的线程
  co_await boost::asio::post(boost::asio::bind_executor(l_this_exe, boost::asio::use_awaitable));
  nlohmann::json l_json_res{};
  l_json_res["data"]     = l_block.task_info_;
  l_json_res["id"]       = fmt::to_string(l_block.id_);
  l_json_res["duration"] = l_block.duration_.count();

  boost::beast::http::response<boost::beast::http::string_body> l_response{
      boost::beast::http::status::ok, in_handle->version_
  };
  l_response.set(boost::beast::http::field::content_type, "application/json");
  l_response.body() = l_json_res.dump();
  l_response.keep_alive(in_handle->keep_alive_);
  l_response.prepare_payload();
  co_return l_response;
}

boost::asio::awaitable<boost::beast::http::message_generator> computing_time_get(session_data_ptr in_handle) {
  auto l_logger = in_handle->logger_;

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

  // 切换到主线程
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
  auto& l_json = std::get<nlohmann::json>(in_handle->body_);

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
  } catch (...) {
    l_logger->log(log_loc(), level::err, boost::current_exception_diagnostic_information());
    co_return in_handle->make_error_code_msg(
      boost::beast::http::status::internal_server_error,
      boost::system::errc::make_error_code(boost::system::errc::bad_message),
      boost::current_exception_diagnostic_information()
    );
  }

  // 保存自己的线程
  auto l_this_exe = co_await boost::asio::this_coro::executor;

  // 切换到主线程
  co_await boost::asio::post(boost::asio::bind_executor(g_strand(), boost::asio::use_awaitable));
  auto [l_user_handle, l_user] = find_user_handle(*g_reg(), l_user_id);
  if (!l_user_handle)
    co_return in_handle->make_error_code_msg(
      boost::beast::http::status::not_found,
      boost::system::error_code{boost::system::errc::bad_message, boost::system::generic_category()}, "找不到用户"
    );
  work_xlsx_task_info_block l_block{};
  entt::handle l_block_handle{};
  if (l_user.task_block_.contains(l_year_month)) {
    l_block_handle = {*g_reg(), l_user.task_block_.at(l_year_month)};
    l_block        = l_block_handle.get<const work_xlsx_task_info_block>();
  } else {
    auto l_year_month_str =
        fmt::format("{}-{}", std::int32_t{l_year_month.year()}, std::uint32_t{l_year_month.month()});
    l_logger->log(log_loc(), level::err, "找不到用户 {} 月份 {}", l_user.mobile_, l_year_month_str);
    co_return in_handle->make_error_code_msg(
      boost::beast::http::status::not_found,
      boost::system::error_code{boost::system::errc::bad_message, boost::system::generic_category()},
      fmt::format("找不到用户 {} 中 {} 月 对应的表格", l_user.mobile_, l_year_month_str)
    );
  }

  // 切换到自己的线程
  co_await boost::asio::post(l_this_exe, boost::asio::bind_executor(l_this_exe, boost::asio::use_awaitable));
  auto l_timer_clock = create_time_clock(l_year_month, l_user);
  auto l_block_opt   = patch_time(l_timer_clock, l_block, l_task_id, l_duration, in_handle->logger_);
  if (l_block_opt) {
    // 切换到主线程
    co_await boost::asio::post(boost::asio::bind_executor(g_strand(), boost::asio::use_awaitable));
    l_block_handle.emplace_or_replace<work_xlsx_task_info_block>(*l_block_opt);
    // 切换到自己的线程
    co_await boost::asio::post(l_this_exe, boost::asio::bind_executor(l_this_exe, boost::asio::use_awaitable));
    l_block = *l_block_opt;
  }
  nlohmann::json l_json_res{};
  l_json_res["data"]     = l_block.task_info_;
  l_json_res["id"]       = fmt::to_string(l_block.id_);
  l_json_res["duration"] = l_block.duration_.count();

  boost::beast::http::response<boost::beast::http::string_body> l_response{
      boost::beast::http::status::ok, in_handle->version_
  };
  l_response.set(boost::beast::http::field::content_type, "application/json");
  l_response.body() = l_json_res.dump();
  l_response.keep_alive(in_handle->keep_alive_);
  l_response.prepare_payload();
  co_return l_response;
}

boost::asio::awaitable<boost::beast::http::message_generator> computing_time_patch_delete(session_data_ptr in_handle) {
  auto l_logger = in_handle->logger_;

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
      // 切换到主注册表线程
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
      ));
}
} // namespace doodle::http
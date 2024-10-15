#include "computing_time.h"

#include "doodle_core/metadata/time_point_wrap.h"
#include "doodle_core/platform/win/register_file_type.h"
#include <doodle_core/metadata/attendance.h>

#include "doodle_lib/core/http/http_session_data.h"

#include <boost/rational.hpp>
//
#include "doodle_core/sqlite_orm/sqlite_database.h"
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

business::work_clock2 create_time_clock(
    const chrono::year_month& in_year_month, const user_helper::database_t& in_user
) {
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

  auto& l_sql = g_ctx().get<sqlite_database>();
  std::vector<chrono::local_days> l_days{};
  for (auto l_it = l_begin_time; l_it <= l_end_time; l_it += chrono::days{1}) l_days.emplace_back(l_it);
  for (auto&& l_att : l_sql.get_attendance(in_user.id_, l_days)) {
    l_time_clock_ += std::make_tuple(l_att.start_time_, l_att.end_time_, l_att.remark_.value_or(std::string{}));
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
void computing_time_run(
    const chrono::year_month& in_year_month, const business::work_clock2& in_time_clock,
    const user_helper::database_t& in_user, computing_time_post_req_data& in_data,
    std::vector<work_xlsx_task_info_helper::database_t>& in_out_data
) {
  auto l_end_time  = chrono::local_days{(in_year_month + chrono::months{1}) / chrono::day{1}} - chrono::seconds{1};
  auto l_all_works = in_time_clock(chrono::local_days{in_year_month / chrono::day{1}}, l_end_time);

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

    chrono::local_time_pos l_begin_time{chrono::local_days{in_year_month / chrono::day{1}}};
    for (auto i = 0; i < in_data.data.size(); ++i) {
      auto l_end           = in_time_clock.next_time(l_begin_time, l_woeks2[i]);

      auto l_info          = in_time_clock.get_time_info(l_begin_time, l_end);
      std::string l_remark = fmt::format("{}", fmt::join(l_info, ", "));

      if (i < in_out_data.size()) {
        in_out_data[i].start_time_        = {chrono::current_zone(), l_begin_time};
        in_out_data[i].end_time_          = {chrono::current_zone(), l_end};
        in_out_data[i].duration_          = chrono::duration_cast<chrono::seconds>(l_woeks2[i]);
        in_out_data[i].remark_            = l_remark;
        in_out_data[i].year_month_        = chrono::local_days{in_year_month / 1};
        in_out_data[i].user_ref_          = in_user.id_;
        in_out_data[i].kitsu_task_ref_id_ = in_data.data[i].task_id;
      } else {
        in_out_data.emplace_back(work_xlsx_task_info_helper::database_t{
            .uuid_id_           = core_set::get_set().get_uuid(),
            // .task_info_  = std::vector<work_xlsx_task_info>{},
            .start_time_        = {chrono::current_zone(), l_begin_time},
            .end_time_          = {chrono::current_zone(), l_end},
            .duration_          = chrono::duration_cast<chrono::seconds>(l_woeks2[i]),
            .remark_            = l_remark,
            .year_month_        = chrono::local_days{in_year_month / 1},
            .user_ref_          = in_user.id_,
            .kitsu_task_ref_id_ = in_data.data[i].task_id
        });
      }
      l_begin_time = l_end;
    }
  }
}

bool patch_time(
    const business::work_clock2& in_time_clock, std::vector<work_xlsx_task_info_helper::database_t>& in_block,
    const boost::uuids::uuid& in_task_id_, const chrono::microseconds& in_duration, const logger_ptr& in_logger_ptr
) {
  auto l_task_it =
      std::ranges::find_if(in_block, [&in_task_id_](const auto& l_task) { return l_task.uuid_id_ == in_task_id_; });
  if (l_task_it == std::end(in_block)) {
    in_logger_ptr->log(log_loc(), level::err, "task {} not found", in_task_id_);
    return false;
  }

  // 只有一个任务, 不可以调整
  if (in_block.size() == 1) return false;

  {
    using rational_int = boost::rational<std::int64_t>;
    rational_int l_duration_int{
        (l_task_it->duration_ - in_duration).count(), boost::numeric_cast<std::int64_t>(in_block.size() - 1)
    };
    // 调整差值
    l_task_it->duration_ = in_duration;
    for (auto&& l_task : in_block) {
      if (l_task.uuid_id_ != in_task_id_) {
        l_task.duration_ += chrono::microseconds{boost::rational_cast<std::int64_t>(l_duration_int)};
      }
    }

    // 计算时间开始结束
    chrono::local_time_pos l_begin_time{in_block[0].year_month_};
    for (auto i = 0; i < in_block.size(); ++i) {
      auto l_end              = in_time_clock.next_time(l_begin_time, in_block[i].duration_);
      in_block[i].start_time_ = l_begin_time;
      in_block[i].end_time_   = l_end;
      l_begin_time            = l_end;
    }
  }
  return true;
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

  user_helper::database_t l_user{};

  if (auto l_users = g_ctx().get<sqlite_database>().get_by_uuid<user_helper::database_t>(l_data.user_id);
      l_users.empty())
    co_return in_handle->make_error_code_msg(
        boost::beast::http::status::not_found,
        boost::system::error_code{boost::system::errc::bad_message, boost::system::generic_category()}, "找不到用户"
    );
  else
    l_user = std::move(*l_users.begin());
  auto l_block_ptr = std::make_shared<std::vector<work_xlsx_task_info_helper::database_t>>();
  *l_block_ptr =
      g_ctx().get<sqlite_database>().get_work_xlsx_task_info(l_user.id_, chrono::local_days{l_data.year_month_ / 1});

  auto l_time_clock = create_time_clock(l_data.year_month_, l_user);
  computing_time_run(l_data.year_month_, l_time_clock, l_user, l_data, *l_block_ptr);

  if (auto l_r = co_await g_ctx().get<sqlite_database>().install_range(l_block_ptr); !l_r) {
    co_return in_handle->make_error_code_msg(boost::beast::http::status::internal_server_error, l_r.error());
  }

  nlohmann::json l_json_res{};
  l_json_res["data"] = *l_block_ptr;
  co_return in_handle->make_msg(l_json_res.dump());
}

boost::asio::awaitable<boost::beast::http::message_generator> computing_time_get(session_data_ptr in_handle) {
  auto l_logger = in_handle->logger_;

  boost::uuids::uuid l_user_id{};
  chrono::year_month l_year_month{};
  try {
    auto l_user_str       = in_handle->capture_->get("user_id");
    auto l_year_month_str = in_handle->capture_->get("year_month");
    l_user_id             = boost::lexical_cast<boost::uuids::uuid>(l_user_str);
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

  user_helper::database_t l_user{};

  if (auto l_users = g_ctx().get<sqlite_database>().get_by_uuid<user_helper::database_t>(l_user_id); l_users.empty())
    co_return in_handle->make_error_code_msg(
        boost::beast::http::status::not_found,
        boost::system::error_code{boost::system::errc::bad_message, boost::system::generic_category()}, "找不到用户"
    );
  else
    l_user = std::move(*l_users.begin());

  auto l_v = g_ctx().get<sqlite_database>().get_work_xlsx_task_info(l_user.id_, chrono::local_days{l_year_month / 1});

  nlohmann::json l_json{};
  l_json["data"] = l_v;
  co_return in_handle->make_msg(l_json.dump());
}

boost::asio::awaitable<boost::beast::http::message_generator> computing_time_patch(session_data_ptr in_handle) {
  auto l_logger = in_handle->logger_;

  if (in_handle->content_type_ != http::detail::content_type::application_json)
    co_return in_handle->make_error_code_msg(
        boost::beast::http::status::bad_request, boost::system::errc::make_error_code(boost::system::errc::bad_message),
        "不是json请求"
    );
  auto& l_json = std::get<nlohmann::json>(in_handle->body_);

  boost::uuids::uuid l_user_id{}, l_task_id{};
  chrono::year_month l_year_month{};
  chrono::microseconds l_duration{};
  try {
    auto l_user_str       = in_handle->capture_->get("user_id");
    auto l_year_month_str = in_handle->capture_->get("year_month");
    auto l_task_id_str    = in_handle->capture_->get("task_id");
    l_task_id             = boost::lexical_cast<boost::uuids::uuid>(l_task_id_str);
    l_user_id             = boost::lexical_cast<boost::uuids::uuid>(l_user_str);
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
  user_helper::database_t l_user{};

  if (auto l_users = g_ctx().get<sqlite_database>().get_by_uuid<user_helper::database_t>(l_user_id); l_users.empty())
    co_return in_handle->make_error_code_msg(
        boost::beast::http::status::not_found,
        boost::system::error_code{boost::system::errc::bad_message, boost::system::generic_category()}, "找不到用户"
    );
  else
    l_user = std::move(*l_users.begin());

  auto l_block_ptr = std::make_shared<std::vector<work_xlsx_task_info_helper::database_t>>();
  *l_block_ptr =
      g_ctx().get<sqlite_database>().get_work_xlsx_task_info(l_user.id_, chrono::local_days{l_year_month / 1});

  if (l_block_ptr->empty()) {
    auto l_year_month_str_1 =
        fmt::format("{}-{}", std::int32_t{l_year_month.year()}, std::uint32_t{l_year_month.month()});
    l_logger->log(log_loc(), level::err, "找不到用户 {} 月份 {}", l_user.mobile_, l_year_month_str_1);
    co_return in_handle->make_error_code_msg(
        boost::beast::http::status::not_found,
        boost::system::error_code{boost::system::errc::bad_message, boost::system::generic_category()},
        fmt::format("找不到用户 {} 中 {} 月 对应的表格", l_user.mobile_, l_year_month_str_1)
    );
  }

  auto l_timer_clock = create_time_clock(l_year_month, l_user);
  if (patch_time(l_timer_clock, *l_block_ptr, l_task_id, l_duration, in_handle->logger_)) {
    if (auto l_r = co_await g_ctx().get<sqlite_database>().install_range(l_block_ptr); !l_r)
      co_return in_handle->make_error_code_msg(boost::beast::http::status::internal_server_error, l_r.error());
  }
  nlohmann::json l_json_res{};
  l_json_res["data"] = *l_block_ptr;

  co_return in_handle->make_msg(l_json_res.dump());
}

boost::asio::awaitable<boost::beast::http::message_generator> computing_time_patch_delete(session_data_ptr in_handle) {
  auto l_logger = in_handle->logger_;

  boost::uuids::uuid l_computing_time_id{};
  try {
    auto l_computing_time_id_str = in_handle->capture_->get("computing_time_id");
    l_computing_time_id          = boost::lexical_cast<boost::uuids::uuid>(l_computing_time_id_str);
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

  auto l_rem = g_ctx().get<sqlite_database>().get_by_uuid<work_xlsx_task_info_helper::database_t>(l_computing_time_id);
  if (!l_rem.empty()) {
    if (auto l_r = co_await g_ctx().get<sqlite_database>().remove<work_xlsx_task_info_helper::database_t>(
            std::make_shared<std::vector<std::int64_t>>(std::vector<std::int64_t>{l_rem.front().id_})
        );
        !l_r) {
      co_return in_handle->make_error_code_msg(boost::beast::http::status::internal_server_error, l_r.error());
    }
  }
  co_return in_handle->make_msg("{}"s);
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
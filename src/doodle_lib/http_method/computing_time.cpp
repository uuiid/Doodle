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

#include <doodle_lib/core/cache_manger.h>
#include <doodle_lib/core/holidaycn_time.h>
#include <doodle_lib/http_client/kitsu_client.h>
#include <doodle_lib/http_method/kitsu/kitsu.h>

namespace doodle::http {
struct computing_time_post_req_data {
  struct task_data {
    boost::uuids::uuid task_id;
    chrono::local_time_pos start_time;
    chrono::local_time_pos end_time;
    // form json
    friend void from_json(const nlohmann::json& j, task_data& p) {
      p.task_id         = boost::lexical_cast<boost::uuids::uuid>(j.at("task_id").get<std::string>());
      auto l_start_time = parse_8601<chrono::local_time_pos>(j.at("start_time").get<std::string>());
      auto l_end_time   = parse_8601<chrono::local_time_pos>(j.at("end_time").get<std::string>());
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

struct computing_time_post_req_custom_data {
  uuid project_id;
  std::int32_t season;
  std::int32_t episode;
  std::string name;
  std::string grade;
  std::string remark;
  chrono::local_time_pos start_time;
  chrono::local_time_pos end_time;
  chrono::year_month year_month_;
  boost::uuids::uuid user_id_;

  friend void from_json(const nlohmann::json& j, computing_time_post_req_custom_data& p) {
    j.at("project_id").get_to(p.project_id);
    j.at("season").get_to(p.season);
    j.at("episode").get_to(p.episode);
    j.at("name").get_to(p.name);
    j.at("grade").get_to(p.grade);
    j.at("user_remark").get_to(p.remark);
    j.at("start_time").get_to(p.start_time);
    j.at("end_time").get_to(p.end_time);

    // 检查数据
    if (p.start_time > p.end_time)
      throw_exception(http_request_error{boost::beast::http::status::bad_request, "开始时间大于结束时间"});
    if (p.project_id.is_nil())
      throw_exception(http_request_error{boost::beast::http::status::bad_request, "project_id 不能为空"});
  }
};

business::work_clock2 create_time_clock(const chrono::year_month& in_year_month, const std::int64_t& in_user_id) {
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
  holidaycn_time2{l_rules_.work_pair_p, g_ctx().get<kitsu_ctx_t>().front_end_root_ / "time"}.set_clock(l_time_clock_);

  auto& l_sql = g_ctx().get<sqlite_database>();
  std::vector<chrono::local_days> l_days{};
  for (auto l_it = l_begin_time; l_it <= l_end_time; l_it += chrono::days{1}) l_days.emplace_back(l_it);
  for (auto&& l_att : l_sql.get_attendance(in_user_id, l_days)) {
    switch (l_att.type_) {
      case attendance_helper::att_enum::overtime:
        l_time_clock_ += std::make_tuple(l_att.start_time_, l_att.end_time_, l_att.remark_);
        break;
      case attendance_helper::att_enum::leave:
        l_time_clock_ -= std::make_tuple(l_att.start_time_, l_att.end_time_, l_att.remark_);
        break;
      case attendance_helper::att_enum::max:
        break;
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
void computing_time_run(
    const chrono::year_month& in_year_month, const business::work_clock2& in_time_clock,
    const user_helper::database_t& in_user, computing_time_post_req_data& in_data,
    std::vector<work_xlsx_task_info_helper::database_t>& in_out_data
) {
  auto l_end_time = chrono::local_days{(in_year_month + chrono::months{1}) / chrono::day{1}} - chrono::seconds{1};
  chrono::local_time_pos l_begin_time{chrono::local_days{in_year_month / chrono::day{1}} + chrono::seconds{1}};
  auto l_all_works = in_time_clock(l_begin_time, l_end_time);

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
      l_woeks2.push_back(
          chrono::seconds{
              boost::rational_cast<std::int64_t>(rational_int{l_woeks1[i], l_works_accumulate} * l_all_works.count())
          }
      );
    }

    for (auto i = 0; i < in_data.data.size(); ++i) {
      auto l_end = in_time_clock.next_time(l_begin_time, l_woeks2[i]);
      if (i + 1 == in_data.data.size()) l_end = l_end_time;

      auto l_info          = in_time_clock.get_time_info(l_begin_time, l_end);
      std::string l_remark = fmt::format("{}", fmt::join(l_info, ", "));

      if (i < in_out_data.size()) {
        in_out_data[i].start_time_        = {chrono::current_zone(), l_begin_time};
        in_out_data[i].end_time_          = {chrono::current_zone(), l_end};
        in_out_data[i].duration_          = in_time_clock(l_begin_time, l_end);
        in_out_data[i].remark_            = l_remark;
        in_out_data[i].year_month_        = chrono::local_days{in_year_month / 1};
        in_out_data[i].user_ref_          = in_user.id_;
        in_out_data[i].kitsu_task_ref_id_ = in_data.data[i].task_id;
      } else {
        in_out_data.emplace_back(
            work_xlsx_task_info_helper::database_t{
                .uuid_id_           = core_set::get_set().get_uuid(),
                // .task_info_  = std::vector<work_xlsx_task_info>{},
                .start_time_        = {chrono::current_zone(), l_begin_time},
                .end_time_          = {chrono::current_zone(), l_end},
                .duration_          = in_time_clock(l_begin_time, l_end),
                .remark_            = l_remark,
                .year_month_        = chrono::local_days{in_year_month / 1},
                .user_ref_          = in_user.id_,
                .kitsu_task_ref_id_ = in_data.data[i].task_id
            }
        );
      }
      l_begin_time = l_end;
    }
  }
}

// 重新计算时间
void recomputing_time_run(
    const chrono::year_month& in_year_month, const business::work_clock2& in_time_clock,
    std::vector<work_xlsx_task_info_helper::database_t>& in_out_data
) {
  auto l_end_time  = chrono::local_days{(in_year_month + chrono::months{1}) / chrono::day{1}} - chrono::seconds{1};
  auto l_all_works = in_time_clock(chrono::local_days{in_year_month / chrono::day{1}}, l_end_time);

  // 进行排序
  std::ranges::sort(in_out_data, [](auto&& l_left, auto&& l_right) {
    return l_left.start_time_.get_sys_time() < l_right.start_time_.get_sys_time();
  });
  // });

  {
    // 计算时间比例
    std::vector<std::int64_t> l_woeks1{};
    for (auto&& l_task : in_out_data) {
      l_woeks1.push_back((l_task.start_time_.get_sys_time() - l_task.end_time_.get_sys_time()).count() + 1);
    }
    auto l_works_accumulate = ranges::accumulate(l_woeks1, std::int64_t{});
    std::vector<chrono::seconds> l_woeks2{};
    using rational_int = boost::rational<std::int64_t>;
    for (auto i = 0; i < l_woeks1.size(); ++i) {
      l_woeks2.push_back(
          chrono::seconds{
              boost::rational_cast<std::int64_t>(rational_int{l_woeks1[i], l_works_accumulate} * l_all_works.count())
          }
      );
    }

    chrono::local_time_pos l_begin_time{chrono::local_days{in_year_month / chrono::day{1}} + chrono::seconds{1}};
    for (auto i = 0; i < in_out_data.size(); ++i) {
      auto l_end = in_time_clock.next_time(l_begin_time, l_woeks2[i]);
      if (i + 1 == in_out_data.size()) l_end = l_end_time;
      auto l_info                = in_time_clock.get_time_info(l_begin_time, l_end);
      std::string l_remark       = fmt::format("{}", fmt::join(l_info, ", "));
      in_out_data[i].start_time_ = {chrono::current_zone(), l_begin_time};
      in_out_data[i].end_time_   = {chrono::current_zone(), l_end};
      in_out_data[i].duration_   = in_time_clock(l_begin_time, l_end);
      in_out_data[i].remark_     = l_remark;
      in_out_data[i].year_month_ = chrono::local_days{in_year_month / 1};
      l_begin_time               = l_end;
    }
  }
}

std::string patch_time(
    const business::work_clock2& in_time_clock, std::vector<work_xlsx_task_info_helper::database_t>& in_block,
    const boost::uuids::uuid& in_task_id_, const chrono::microseconds& in_duration, const logger_ptr& in_logger_ptr
) {
  auto l_task_it =
      std::ranges::find_if(in_block, [&in_task_id_](const auto& l_task) { return l_task.uuid_id_ == in_task_id_; });
  if (l_task_it == std::end(in_block)) {
    return fmt::format("task {} not found", in_task_id_);
  }

  // 只有一个任务, 不可以调整
  if (in_block.size() == 1) return {};
  auto l_time_begin = chrono::local_time_pos{in_block[0].year_month_};
  auto l_time_end =
      chrono::local_time_pos{chrono::local_days{chrono::year_month_day{in_block[0].year_month_} + chrono::months{1}}} -
      chrono::seconds{1};
  auto l_max = in_time_clock(l_time_begin, l_time_end);
  if (in_duration >= l_max)
    return fmt::format(
        "大于最大时长 {} {} ", boost::numeric_cast<std::double_t>(in_duration.count() / (60ull * 60ull * 8ull)),
        boost::numeric_cast<std::double_t>(l_max.count() / (60ull * 60ull * 8ull))
    );
  if (in_duration <= chrono::microseconds{}) return "时间不可小于0";

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
        l_task.duration_ = std::clamp(l_task.duration_, chrono::microseconds{60s}, chrono::microseconds{l_max});
      }
    }

    // 计算时间开始结束
    chrono::local_time_pos l_begin_time{in_block[0].year_month_};
    for (auto i = 0; i < in_block.size(); ++i) {
      auto l_end = in_time_clock.next_time(l_begin_time, in_block[i].duration_);
      if (l_end >= l_time_end) l_end = l_time_end;
      if (i + 1 == in_block.size()) l_end = l_time_end;
      in_block[i].start_time_ = l_begin_time;
      in_block[i].end_time_   = l_end;
      in_block[i].duration_   = in_time_clock(l_begin_time, l_end);
      l_begin_time            = l_end;
    }
  }
  return {};
}

boost::asio::awaitable<tl::expected<nlohmann::json, std::string>> merge_full_task(
    session_data_ptr in_handle, std::shared_ptr<std::vector<work_xlsx_task_info_helper::database_t>> in_block_ptr
) {
  auto l_c = kitsu::create_kitsu_proxy(in_handle);
  nlohmann::json l_json_res{};

  for (auto&& l_d : *in_block_ptr) {
    if (!l_d.kitsu_task_ref_id_.is_nil()) {
      auto l_cache_json = g_ctx().get<cache_manger>().get(l_d.kitsu_task_ref_id_);
      if (l_cache_json) {
        auto& l_json_v             = l_json_res["data"].emplace_back(*l_cache_json);
        l_json_v["computing_time"] = l_d;
      } else {
        boost::beast::http::request<boost::beast::http::empty_body> l_q{in_handle->req_header_};
        l_q.method(boost::beast::http::verb::get);
        l_q.target(fmt::format("/api/data/tasks/{}/full", l_d.kitsu_task_ref_id_));
        l_q.erase(boost::beast::http::field::content_length);
        l_q.erase(boost::beast::http::field::content_type);
        l_q.keep_alive(in_handle->keep_alive_ == false ? in_block_ptr->size() != 1 : true);
        // std::ostringstream l_os;
        // l_os << l_q;
        // default_logger_raw()->warn("请求数据 {}", l_os.str());
        auto [l_e, l_r] = co_await detail::read_and_write<boost::beast::http::string_body>(l_c, std::move(l_q));
        if (l_e) co_return tl::make_unexpected(l_e.message());
        auto l_json    = nlohmann::json::parse(l_r.body());
        auto& l_json_v = l_json_res["data"].emplace_back(l_json);
        g_ctx().get<cache_manger>().set(l_d.kitsu_task_ref_id_, l_json);
        l_json_v["computing_time"] = l_d;
      }
    } else
      l_json_res["data"].emplace_back()["computing_time"] = l_d;
  }
  co_return tl::expected<nlohmann::json, std::string>{std::move(l_json_res)};
}
boost::asio::awaitable<boost::beast::http::message_generator> computing_time_post(session_data_ptr in_handle) {
  if (in_handle->content_type_ != http::detail::content_type::application_json)
    co_return in_handle->make_error_code_msg(
        boost::beast::http::status::bad_request, boost::system::errc::make_error_code(boost::system::errc::bad_message),
        "不是json请求"
    );

  auto l_json                         = std::get<nlohmann::json>(in_handle->body_);

  computing_time_post_req_data l_data = l_json.get<computing_time_post_req_data>();
  {  // 检查除空以外的id是否重复
    std::map<uuid, std::size_t> l_map;
    for (auto&& l_task : l_data.data) {
      if (l_task.task_id.is_nil())
        co_return in_handle->make_error_code_msg(boost::beast::http::status::bad_request, "task_id 不可为空");
      l_map[l_task.task_id]++;
    }
    if (l_map.contains(uuid{})) l_map.erase(uuid{});
    if (std::ranges::any_of(l_map, [](const auto& p) { return p.second > 1; }))
      co_return in_handle->make_error_code_msg(boost::beast::http::status::bad_request, "提交的task id 有重复");
  }

  l_data.user_id = boost::lexical_cast<boost::uuids::uuid>(in_handle->capture_->get("user_id"));
  std::istringstream l_year_month_stream{in_handle->capture_->get("year_month")};
  l_year_month_stream >> chrono::parse("%Y-%m", l_data.year_month_);

  user_helper::database_t l_user = g_ctx().get<sqlite_database>().get_by_uuid<user_helper::database_t>(l_data.user_id);
  auto l_block_ptr               = std::make_shared<std::vector<work_xlsx_task_info_helper::database_t>>();
  *l_block_ptr =
      g_ctx().get<sqlite_database>().get_work_xlsx_task_info(l_user.id_, chrono::local_days{l_data.year_month_ / 1});

  auto l_time_clock = create_time_clock(l_data.year_month_, l_user.id_);
  computing_time_run(l_data.year_month_, l_time_clock, l_user, l_data, *l_block_ptr);

  co_await g_ctx().get<sqlite_database>().install_range(l_block_ptr);

  if (auto l_r = co_await merge_full_task(in_handle, l_block_ptr); !l_r) {
    co_return in_handle->make_error_code_msg(boost::beast::http::status::internal_server_error, l_r.error());
  } else
    co_return in_handle->make_msg(l_r->dump());
}
boost::asio::awaitable<boost::beast::http::message_generator> computing_time_post_add(session_data_ptr in_handle) {
  if (in_handle->content_type_ != http::detail::content_type::application_json)
    co_return in_handle->make_error_code_msg(
        boost::beast::http::status::bad_request, boost::system::errc::make_error_code(boost::system::errc::bad_message),
        "不是json请求"
    );
  auto l_json = std::get<nlohmann::json>(in_handle->body_);
  computing_time_post_req_data l_data{};
  l_data.data.emplace_back(l_json.get<computing_time_post_req_data::task_data>());
  if (l_data.data.front().task_id.is_nil())
    co_return in_handle->make_error_code_msg(boost::beast::http::status::bad_request, "task_id 不可为空");
  l_data.user_id = boost::lexical_cast<boost::uuids::uuid>(in_handle->capture_->get("user_id"));
  std::istringstream l_year_month_stream{in_handle->capture_->get("year_month")};
  l_year_month_stream >> chrono::parse("%Y-%m", l_data.year_month_);

  user_helper::database_t l_user = g_ctx().get<sqlite_database>().get_by_uuid<user_helper::database_t>(l_data.user_id);
  auto l_block_ptr               = std::make_shared<std::vector<work_xlsx_task_info_helper::database_t>>();
  *l_block_ptr =
      g_ctx().get<sqlite_database>().get_work_xlsx_task_info(l_user.id_, chrono::local_days{l_data.year_month_ / 1});
  {
    work_xlsx_task_info_helper::database_t l_data_work{
        .uuid_id_ = core_set::get_set().get_uuid(),
        .start_time_ =
            work_xlsx_task_info_helper::database_t::zoned_time{
                chrono::current_zone(),
                chrono::time_point_cast<work_xlsx_task_info_helper::database_t::zoned_time::duration>(
                    l_data.data.front().start_time
                )
            },
        .end_time_ =
            work_xlsx_task_info_helper::database_t::zoned_time{
                chrono::current_zone(),
                chrono::time_point_cast<work_xlsx_task_info_helper::database_t::zoned_time::duration>(
                    l_data.data.front().end_time
                )
            },
        .year_month_        = chrono::local_days{l_data.year_month_ / 1},
        .user_ref_          = l_user.id_,
        .kitsu_task_ref_id_ = l_data.data.front().task_id,
    };
    chrono::local_time_pos l_end_time =
        chrono::local_days{(l_data.year_month_ + chrono::months{1}) / chrono::day{1}} - chrono::seconds{1};

    chrono::local_time_pos l_begin_time{chrono::local_days{l_data.year_month_ / chrono::day{1}} + chrono::seconds{1}};

    l_data_work.start_time_ = std::clamp(
        chrono::time_point_cast<chrono::local_time_pos::duration>(l_data_work.start_time_.get_local_time()),
        l_begin_time, l_end_time
    );
    l_data_work.end_time_ = std::clamp(
        chrono::time_point_cast<chrono::local_time_pos::duration>(l_data_work.end_time_.get_local_time()), l_begin_time,
        l_end_time
    );
    l_block_ptr->emplace_back(std::move(l_data_work));
  }

  auto l_time_clock = create_time_clock(l_data.year_month_, l_user.id_);
  recomputing_time_run(l_data.year_month_, l_time_clock, *l_block_ptr);
  co_await g_ctx().get<sqlite_database>().install_range(l_block_ptr);

  if (auto l_r = co_await merge_full_task(in_handle, l_block_ptr); !l_r) {
    co_return in_handle->make_error_code_msg(boost::beast::http::status::internal_server_error, l_r.error());
  } else
    co_return in_handle->make_msg(l_r->dump());
}
boost::asio::awaitable<boost::beast::http::message_generator> computing_time_post_custom(session_data_ptr in_handle) {
  if (in_handle->content_type_ != http::detail::content_type::application_json)
    co_return in_handle->make_error_code_msg(
        boost::beast::http::status::bad_request, boost::system::errc::make_error_code(boost::system::errc::bad_message),
        "不是json请求"
    );
  auto l_json                                = std::get<nlohmann::json>(in_handle->body_);
  computing_time_post_req_custom_data l_data = l_json.get<computing_time_post_req_custom_data>();

  l_data.user_id_ = boost::lexical_cast<boost::uuids::uuid>(in_handle->capture_->get("user_id"));
  std::istringstream l_year_month_stream{in_handle->capture_->get("year_month")};
  l_year_month_stream >> chrono::parse("%Y-%m", l_data.year_month_);

  // 检查数据
  if (l_data.start_time > l_data.end_time)
    co_return in_handle->make_error_code_msg(
        boost::beast::http::status::bad_request, boost::system::errc::make_error_code(boost::system::errc::bad_message),
        "开始时间大于结束时间"
    );
  if (l_data.project_id.is_nil())
    co_return in_handle->make_error_code_msg(
        boost::beast::http::status::bad_request, boost::system::errc::make_error_code(boost::system::errc::bad_message),
        "project_id 不可为空"
    );

  user_helper::database_t l_user = g_ctx().get<sqlite_database>().get_by_uuid<user_helper::database_t>(l_data.user_id_);
  auto l_block_ptr               = std::make_shared<std::vector<work_xlsx_task_info_helper::database_t>>();
  *l_block_ptr =
      g_ctx().get<sqlite_database>().get_work_xlsx_task_info(l_user.id_, chrono::local_days{l_data.year_month_ / 1});

  l_block_ptr->emplace_back(
      work_xlsx_task_info_helper::database_t{
          .uuid_id_ = core_set::get_set().get_uuid(),
          .start_time_ =
              work_xlsx_task_info_helper::database_t::zoned_time{
                  chrono::current_zone(),
                  chrono::time_point_cast<work_xlsx_task_info_helper::database_t::zoned_time::duration>(
                      l_data.start_time
                  )
              },
          .end_time_ =
              work_xlsx_task_info_helper::database_t::zoned_time{
                  chrono::current_zone(),
                  chrono::time_point_cast<work_xlsx_task_info_helper::database_t::zoned_time::duration>(l_data.end_time)
              },
          .user_remark_ = l_data.remark,
          .year_month_  = chrono::local_days{l_data.year_month_ / 1},
          .user_ref_    = l_user.id_,
          .season_      = l_data.season,
          .episode_     = l_data.episode,
          .name_        = l_data.name,
          .grade_       = l_data.grade,
          .project_id_  = l_data.project_id
      }
  );

  auto l_time_clock = create_time_clock(l_data.year_month_, l_user.id_);
  recomputing_time_run(l_data.year_month_, l_time_clock, *l_block_ptr);
  co_await g_ctx().get<sqlite_database>().install_range(l_block_ptr);

  if (auto l_r = co_await merge_full_task(in_handle, l_block_ptr); !l_r) {
    co_return in_handle->make_error_code_msg(boost::beast::http::status::internal_server_error, l_r.error());
  } else
    co_return in_handle->make_msg(l_r->dump());
}
boost::asio::awaitable<boost::beast::http::message_generator> computing_time_post_sort(session_data_ptr in_handle) {
  if (in_handle->content_type_ != http::detail::content_type::application_json)
    co_return in_handle->make_error_code_msg(
        boost::beast::http::status::bad_request, boost::system::errc::make_error_code(boost::system::errc::bad_message),
        "不是json请求"
    );
  auto l_json                    = std::get<nlohmann::json>(in_handle->body_);
  auto l_data                    = l_json.get<std::vector<uuid>>();
  auto l_user_id                 = boost::lexical_cast<boost::uuids::uuid>(in_handle->capture_->get("user_id"));
  auto l_year_month              = parse_time<chrono::year_month>(in_handle->capture_->get("year_month"), "%Y-%m");

  user_helper::database_t l_user = g_ctx().get<sqlite_database>().get_by_uuid<user_helper::database_t>(l_user_id);

  const auto l_block =
      g_ctx().get<sqlite_database>().get_work_xlsx_task_info(l_user.id_, chrono::local_days{l_year_month / 1});

  {
    // 检查排序
    if (l_block.size() != l_data.size())
      throw_exception(http_request_error{boost::beast::http::status::bad_request, "排序错误传入的列表不是总列表"});
    std::vector<uuid> l_tmp = l_data;
    std::vector<uuid> l_tmp2 =
        l_block | ranges::views::transform([](auto& in) { return in.uuid_id_; }) | ranges::to_vector;
    l_tmp |= ranges::actions::sort;
    l_tmp2 |= ranges::actions::sort;
    if (l_tmp != l_tmp2)
      throw_exception(http_request_error{boost::beast::http::status::bad_request, "排序列表和总列表id不一致"});
  }

  std::map<uuid, const work_xlsx_task_info_helper::database_t*> l_map =
      l_block | ranges::views::transform([](const work_xlsx_task_info_helper::database_t& in) {
        return std::make_pair(in.uuid_id_, &in);
      }) |
      ranges::to<std::map<uuid, const work_xlsx_task_info_helper::database_t*>>;

  auto l_block_sort = std::make_shared<std::vector<work_xlsx_task_info_helper::database_t>>();

  for (int i = 0; i < l_data.size(); ++i) {
    auto&& l_d      = l_block_sort->emplace_back(*l_map[l_data.at(i)]);
    l_d.start_time_ = l_block.at(i).start_time_;
    l_d.end_time_   = l_block.at(i).end_time_;
  }

  auto l_time_clock = create_time_clock(l_year_month, l_user.id_);
  recomputing_time_run(l_year_month, l_time_clock, *l_block_sort);
  co_await g_ctx().get<sqlite_database>().install_range(l_block_sort);
  nlohmann::json l_json_res{};
  l_json_res["data"] = *l_block_sort;
  co_return in_handle->make_msg(l_json_res.dump());
}
boost::asio::awaitable<boost::beast::http::message_generator> computing_time_get(session_data_ptr in_handle) {
  auto l_logger                   = in_handle->logger_;

  boost::uuids::uuid l_user_id    = boost::lexical_cast<boost::uuids::uuid>(in_handle->capture_->get("user_id"));
  chrono::year_month l_year_month = parse_time<chrono::year_month>(in_handle->capture_->get("year_month"), "%Y-%m");

  user_helper::database_t l_user  = g_ctx().get<sqlite_database>().get_by_uuid<user_helper::database_t>(l_user_id);
  auto l_block_ptr                = std::make_shared<std::vector<work_xlsx_task_info_helper::database_t>>();

  *l_block_ptr =
      g_ctx().get<sqlite_database>().get_work_xlsx_task_info(l_user.id_, chrono::local_days{l_year_month / 1});

  if (auto l_r = co_await merge_full_task(in_handle, l_block_ptr); !l_r) {
    co_return in_handle->make_error_code_msg(boost::beast::http::status::internal_server_error, l_r.error());
  } else
    co_return in_handle->make_msg(l_r->dump());
}

boost::asio::awaitable<boost::beast::http::message_generator> computing_time_patch(session_data_ptr in_handle) {
  auto l_logger = in_handle->logger_;

  if (in_handle->content_type_ != http::detail::content_type::application_json)
    co_return in_handle->make_error_code_msg(
        boost::beast::http::status::bad_request, boost::system::errc::make_error_code(boost::system::errc::bad_message),
        "不是json请求"
    );
  auto& l_json = std::get<nlohmann::json>(in_handle->body_);

  boost::uuids::uuid l_user_id{boost::lexical_cast<boost::uuids::uuid>(in_handle->capture_->get("user_id"))},
      l_task_id{boost::lexical_cast<boost::uuids::uuid>(in_handle->capture_->get("task_id"))};

  chrono::year_month l_year_month = parse_time<chrono::year_month>(in_handle->capture_->get("year_month"), "%Y-%m");

  std::optional<chrono::microseconds> l_duration =
      l_json.contains("duration") ? std::optional{chrono::microseconds{l_json["duration"].get<std::int64_t>()}}
                                  : std::nullopt;
  std::optional<std::string> l_comment =
      l_json.contains("user_remark") ? std::optional{l_json["user_remark"].get<std::string>()} : std::nullopt;

  if (l_duration && l_duration->count() <= 0) {
    co_return in_handle->make_error_code_msg(
        boost::beast::http::status::bad_request,
        boost::system::error_code{boost::system::errc::bad_message, boost::system::generic_category()}, "参数错误"
    );
  }

  user_helper::database_t l_user = g_ctx().get<sqlite_database>().get_by_uuid<user_helper::database_t>(l_user_id);
  auto l_block_ptr               = std::make_shared<std::vector<work_xlsx_task_info_helper::database_t>>();
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
  if (l_duration) {
    auto l_timer_clock = create_time_clock(l_year_month, l_user.id_);
    if (auto l_err = patch_time(l_timer_clock, *l_block_ptr, l_task_id, *l_duration, in_handle->logger_);
        l_err.empty()) {
      co_await g_ctx().get<sqlite_database>().install_range(l_block_ptr);
    } else {
      co_return in_handle->make_error_code_msg(boost::beast::http::status::bad_request, l_err);
    }
  } else if (l_comment) {
    auto l_block_ptr_value = std::make_shared<work_xlsx_task_info_helper::database_t>();
    for (auto&& l_b : *l_block_ptr) {
      if (l_b.uuid_id_ == l_task_id) {
        l_b.user_remark_   = *l_comment;
        *l_block_ptr_value = l_b;
        break;
      }
    }
    co_await g_ctx().get<sqlite_database>().install(l_block_ptr_value);
  }
  nlohmann::json l_json_res{};
  l_json_res["data"] = *l_block_ptr;

  co_return in_handle->make_msg(l_json_res.dump());
}

boost::asio::awaitable<boost::beast::http::message_generator> computing_time_patch_delete(session_data_ptr in_handle) {
  auto l_logger = in_handle->logger_;

  boost::uuids::uuid l_computing_time_id =
      boost::lexical_cast<boost::uuids::uuid>(in_handle->capture_->get("computing_time_id"));

  work_xlsx_task_info_helper::database_t l_task =
      g_ctx().get<sqlite_database>().get_by_uuid<work_xlsx_task_info_helper::database_t>(l_computing_time_id);
  nlohmann::json l_json_res{};
  co_await g_ctx().get<sqlite_database>().remove<work_xlsx_task_info_helper::database_t>(
      std::make_shared<std::vector<std::int64_t>>(std::vector<std::int64_t>{l_task.id_})
  );
  chrono::year_month_day l_year_month_day{l_task.year_month_};
  chrono::year_month l_year_month{l_year_month_day.year(), l_year_month_day.month()};

  auto l_block_ptr  = std::make_shared<std::vector<work_xlsx_task_info_helper::database_t>>();
  *l_block_ptr      = g_ctx().get<sqlite_database>().get_work_xlsx_task_info(l_task.user_ref_, l_task.year_month_);

  auto l_time_clock = create_time_clock(l_year_month, l_task.user_ref_);
  recomputing_time_run(l_year_month, l_time_clock, *l_block_ptr);

  co_await g_ctx().get<sqlite_database>().install_range(l_block_ptr);

  l_json_res["data"] = *l_block_ptr;
  co_return in_handle->make_msg(l_json_res.dump());
}

boost::asio::awaitable<void> recomputing_time(
    const std::shared_ptr<user_helper::database_t>& in_user, const chrono::year_month& in_year_month
) {
  auto l_block_ptr = std::make_shared<std::vector<work_xlsx_task_info_helper::database_t>>();
  *l_block_ptr =
      g_ctx().get<sqlite_database>().get_work_xlsx_task_info(in_user->id_, chrono::local_days{in_year_month / 1});
  auto l_timer_clock = create_time_clock(in_year_month, in_user->id_);
  recomputing_time_run(in_year_month, l_timer_clock, *l_block_ptr);
  co_return co_await g_ctx().get<sqlite_database>().install_range(l_block_ptr);
}

void reg_computing_time(http_route& in_route) {
  in_route
      .reg(
          std::make_shared<http_function>(
              boost::beast::http::verb::post, "api/doodle/computing_time/{user_id}/{year_month}", computing_time_post
          )
      )
      .reg(
          std::make_shared<http_function>(
              boost::beast::http::verb::post, "api/doodle/computing_time/{user_id}/{year_month}/add",
              computing_time_post_add
          )
      )
      .reg(
          std::make_shared<http_function>(
              boost::beast::http::verb::post, "api/doodle/computing_time/{user_id}/{year_month}/custom",
              computing_time_post_custom
          )
      )
      .reg(
          std::make_shared<http_function>(
              boost::beast::http::verb::post, "api/doodle/computing_time/{user_id}/{year_month}/sort",
              computing_time_post_sort
          )
      )
      .reg(
          std::make_shared<http_function>(
              boost::beast::http::verb::get, "api/doodle/computing_time/{user_id}/{year_month}", computing_time_get
          )
      )
      .reg(
          std::make_shared<http_function>(
              boost::beast::http::verb::patch, "api/doodle/computing_time/{user_id}/{year_month}/{task_id}",
              computing_time_patch
          )
      )
      .reg(
          std::make_shared<http_function>(
              boost::beast::http::verb::delete_, "api/doodle/computing_time/{computing_time_id}",
              computing_time_patch_delete
          )
      );
}
}  // namespace doodle::http
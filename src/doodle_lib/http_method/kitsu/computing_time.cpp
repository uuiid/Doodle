#include "computing_time.h"

#include "doodle_core/exception/exception.h"
#include "doodle_core/metadata/time_point_wrap.h"
#include "doodle_core/platform/win/register_file_type.h"
#include <doodle_core/metadata/attendance.h>
#include <doodle_core/metadata/entity_type.h>

#include "doodle_lib/core/http/http_session_data.h"

#include <boost/rational.hpp>

#include <memory>

//
#include "doodle_core/sqlite_orm/sqlite_database.h"
#include <doodle_core/metadata/user.h>
#include <doodle_core/metadata/work_xlsx_task_info.h>
#include <doodle_core/sqlite_orm/detail/sqlite_database_impl.h>
#include <doodle_core/time_tool/work_clock.h>

#include <doodle_lib/core/cache_manger.h>
#include <doodle_lib/core/holidaycn_time.h>
#include <doodle_lib/http_client/kitsu_client.h>
#include <doodle_lib/http_method/kitsu.h>
namespace doodle::http {
namespace {
struct work_xlsx_task_info_helper_t {
  decltype(task::uuid_id_) task_id_;
  decltype(task::name_) task_name_;
  decltype(task::last_preview_file_id_) task_last_preview_file_id_;

  decltype(entity::uuid_id_) entity_id_;
  decltype(entity::name_) entity_name_;

  decltype(entity_asset_extend::ji_shu_lie_) entity_ji_shu_lie_;
  decltype(entity_asset_extend::deng_ji_) entity_deng_ji_;
  decltype(entity_asset_extend::gui_dang_) entity_gui_dang_;
  decltype(entity_asset_extend::bian_hao_) entity_bian_hao_;
  decltype(entity_asset_extend::pin_yin_ming_cheng_) entity_pin_yin_ming_cheng_;
  decltype(entity_asset_extend::ban_ben_) entity_ban_ben_;
  decltype(entity_asset_extend::ji_du_) entity_ji_du_;
  decltype(entity_asset_extend::kai_shi_ji_shu_) entity_kai_shi_ji_shu_;

  decltype(task_type::uuid_id_) task_type_id_;

  decltype(project::uuid_id_) project_uuid_;
  decltype(project::name_) project_name_;

  decltype(work_xlsx_task_info_helper::database_t::uuid_id_) id_;
  decltype(work_xlsx_task_info_helper::database_t::start_time_) work_start_time_;
  decltype(work_xlsx_task_info_helper::database_t::end_time_) work_end_time_;
  decltype(work_xlsx_task_info_helper::database_t::duration_) work_duration_;
  decltype(work_xlsx_task_info_helper::database_t::remark_) work_remark_;
  decltype(work_xlsx_task_info_helper::database_t::user_remark_) work_user_remark_;
  decltype(work_xlsx_task_info_helper::database_t::year_month_) work_year_month_;
  decltype(work_xlsx_task_info_helper::database_t::grade_) work_grade_;

  // to json
  friend void to_json(nlohmann::json& j, const work_xlsx_task_info_helper_t& p) {
    j["task_id"]                   = p.task_id_;
    j["task_name"]                 = p.task_name_;
    j["task_last_preview_file_id"] = p.task_last_preview_file_id_;
    j["entity_id"]                 = p.entity_id_;
    j["entity_name"]               = p.entity_name_;

    j["entity_ji_shu_lie"]         = p.entity_ji_shu_lie_;
    j["entity_deng_ji"]            = p.entity_deng_ji_;
    j["entity_gui_dang"]           = p.entity_gui_dang_;
    j["entity_bian_hao"]           = p.entity_bian_hao_;
    j["entity_pin_yin_ming_cheng"] = p.entity_pin_yin_ming_cheng_;
    j["entity_ban_ben"]            = p.entity_ban_ben_;
    j["entity_ji_du"]              = p.entity_ji_du_;
    j["entity_kai_shi_ji_shu"]     = p.entity_kai_shi_ji_shu_;
    j["task_type_id"]              = p.task_type_id_;
    j["project_uuid"]              = p.project_uuid_;
    j["project_name"]              = p.project_name_;
    j["id"]                        = p.id_;
    j["work_start_time"]           = p.work_start_time_;
    j["work_end_time"]             = p.work_end_time_;
    j["work_duration"]             = p.work_duration_;
    j["work_remark"]               = p.work_remark_;
    j["work_user_remark"]          = p.work_user_remark_;
    j["work_year_month"]           = p.work_year_month_;
  }
};

std::vector<work_xlsx_task_info_helper_t> get_task_fulls(
    const std::vector<work_xlsx_task_info_helper::database_t>& in_data
) {
  std::vector<work_xlsx_task_info_helper_t> l_ret{};
  std::vector<uuid> l_task_ids{};
  std::map<uuid, const work_xlsx_task_info_helper::database_t*> l_task_id_map{};
  for (auto&& l_item : in_data) l_task_id_map.emplace(l_item.kitsu_task_ref_id_, &l_item);
  std::map<uuid, std::string> l_project_name_map{};

  l_task_ids.reserve(in_data.size());
  for (auto&& l_item : in_data)
    if (!l_item.kitsu_task_ref_id_.is_nil()) l_task_ids.emplace_back(l_item.kitsu_task_ref_id_);
  auto l_sql = g_ctx().get<sqlite_database>();
  using namespace sqlite_orm;
  for (auto&& [uuid, name] : l_sql.impl_->storage_any_.select(columns(&project::uuid_id_, &project::name_)))
    l_project_name_map.emplace(uuid, name);

  // 先加载自定义task
  for (auto&& l_item : in_data) {
    if (!l_item.kitsu_task_ref_id_.is_nil()) continue;
    l_ret.emplace_back(
        work_xlsx_task_info_helper_t{
            .task_name_         = l_item.name_,
            .entity_ji_shu_lie_ = l_item.episode_,
            .entity_deng_ji_    = l_item.grade_,
            .entity_ji_du_      = l_item.season_,
            .project_uuid_      = l_item.project_id_,
            .project_name_ =
                !l_item.project_id_.is_nil() ? l_project_name_map.at(l_item.project_id_) : l_item.project_name_,
            .id_               = l_item.uuid_id_,
            .work_start_time_  = l_item.start_time_,
            .work_end_time_    = l_item.end_time_,
            .work_duration_    = l_item.duration_,
            .work_remark_      = l_item.remark_,
            .work_user_remark_ = l_item.user_remark_,
            .work_year_month_  = l_item.year_month_,
        }
    );
  }

  for (auto&& [

           task_id_, task_name_, task_last_preview_file_id_,

           entity_id_, entity_name_, task_type_id_,

           entity_ji_shu_lie_, entity_deng_ji_, entity_gui_dang_, entity_bian_hao_, entity_pin_yin_ming_cheng_,
           entity_ban_ben_, entity_ji_du_, entity_kai_shi_ji_shu_,

           project_uuid_, project_name_

  ] :
       l_sql.impl_->storage_any_.select(
           columns(
               &task::uuid_id_, &task::name_, &task::last_preview_file_id_,

               &entity::uuid_id_, &entity::name_, &task_type::uuid_id_,

               &entity_asset_extend::ji_shu_lie_, &entity_asset_extend::deng_ji_, &entity_asset_extend::gui_dang_,
               &entity_asset_extend::bian_hao_, &entity_asset_extend::pin_yin_ming_cheng_,
               &entity_asset_extend::ban_ben_, &entity_asset_extend::ji_du_, &entity_asset_extend::kai_shi_ji_shu_,

               &project::uuid_id_, &project::name_

           ),
           from<task>(), join<entity>(on(c(&task::entity_id_) == c(&entity::uuid_id_))),
           join<task_type>(on(c(&task_type::uuid_id_) == c(&task::task_type_id_))),
           left_outer_join<entity_asset_extend>(on(&entity_asset_extend::entity_id_) == c(&entity::uuid_id_)),
           join<project>(on(c(&project::uuid_id_) == c(&task::project_id_))), where(in(&task::uuid_id_, l_task_ids))
       )) {
    l_ret.emplace_back(
        work_xlsx_task_info_helper_t{
            .task_id_                   = task_id_,
            .task_name_                 = task_name_,
            .task_last_preview_file_id_ = task_last_preview_file_id_,

            .entity_id_                 = entity_id_,
            .entity_name_               = entity_name_,

            .entity_ji_shu_lie_         = entity_ji_shu_lie_,
            .entity_deng_ji_            = entity_deng_ji_,
            .entity_gui_dang_           = entity_gui_dang_,
            .entity_bian_hao_           = entity_bian_hao_,
            .entity_pin_yin_ming_cheng_ = entity_pin_yin_ming_cheng_,
            .entity_ban_ben_            = entity_ban_ben_,
            .entity_ji_du_              = entity_ji_du_,
            .entity_kai_shi_ji_shu_     = entity_kai_shi_ji_shu_,
            .task_type_id_              = task_type_id_,

            .project_uuid_              = project_uuid_,
            .project_name_              = project_name_,
            .id_                        = l_task_id_map.at(task_id_)->uuid_id_,
            .work_start_time_           = l_task_id_map.at(task_id_)->start_time_,
            .work_end_time_             = l_task_id_map.at(task_id_)->end_time_,
            .work_duration_             = l_task_id_map.at(task_id_)->duration_,
            .work_remark_               = l_task_id_map.at(task_id_)->remark_,
            .work_user_remark_          = l_task_id_map.at(task_id_)->user_remark_,
            .work_year_month_           = l_task_id_map.at(task_id_)->year_month_
        }
    );
  }

  l_ret |=
      ranges::actions::sort([](const work_xlsx_task_info_helper_t& l_lhs, const work_xlsx_task_info_helper_t& l_rhs) {
        return l_lhs.work_start_time_.get_sys_time() < l_rhs.work_start_time_.get_sys_time();
      });
  return l_ret;
}
}  // namespace

struct computing_time_post_req_data {
  boost::uuids::uuid task_id;
  chrono::local_time_pos start_time;
  chrono::local_time_pos end_time;
  // form json
  friend void from_json(const nlohmann::json& j, computing_time_post_req_data& p) {
    j.at("task_id").get_to(p.task_id);
    j.at("work_start_time").get_to(p.start_time);
    j.at("work_end_time").get_to(p.end_time);

    if (p.start_time > p.end_time) std::swap(p.start_time, p.end_time);
    if (p.task_id.is_nil())
      throw_exception(http_request_error{boost::beast::http::status::bad_request, "task_id is nil"});
  }
};

struct computing_time_post_req_custom_data {
  uuid project_id;
  std::string project_name_;
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
    if (j.contains("project_uuid"))
      j.at("project_uuid").get_to(p.project_id);
    else
      j.at("project_name").get_to(p.project_name_);

    j.at("entity_ji_du").get_to(p.season);
    j.at("entity_ji_shu_lie").get_to(p.episode);
    j.at("task_name").get_to(p.name);
    if (j.contains("entity_deng_ji")) j.at("entity_deng_ji").get_to(p.grade);
    j.at("work_user_remark").get_to(p.remark);
    j.at("work_start_time").get_to(p.start_time);
    j.at("work_end_time").get_to(p.end_time);

    // 检查数据
    if (p.start_time > p.end_time)
      throw_exception(http_request_error{boost::beast::http::status::bad_request, "开始时间大于结束时间"});
    if (p.project_id.is_nil() && p.project_name_.empty())
      throw_exception(http_request_error{boost::beast::http::status::bad_request, "project_id 不能为空"});
  }
};

business::work_clock2 create_time_clock(const chrono::year_month& in_year_month, const uuid& in_user_id) {
  business::work_clock2 l_time_clock_{};
  auto l_rules_ = business::rules::get_default();
  chrono::local_days l_begin_time{chrono::local_days{in_year_month / chrono::day{1}}},
      l_end_time{chrono::local_days{in_year_month / chrono::last} + chrono::days{3}};
  holidaycn_time2 l_holidaycn_time{l_rules_.work_pair_p, g_ctx().get<kitsu_ctx_t>().front_end_root_ / "time"};
  for (auto l_begin = l_begin_time; l_begin <= l_end_time; l_begin += chrono::days{1}) {
    // 加入工作日规定时间
    if (l_holidaycn_time.is_working_day(l_begin)) {
      for (auto&& l_work_time : l_rules_.work_pair_p) {
        l_time_clock_ += std::make_tuple(l_begin + l_work_time.first, l_begin + l_work_time.second);
      }
    }
  }
  // 调整节假日
  l_holidaycn_time.set_clock(l_time_clock_);

  // 调整请假等调整
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
    for (auto&& l_deduction :
         (l_holidaycn_time.is_working_day(l_begin) ? l_rules_.work_pair_1_ : l_rules_.work_pair_0_))
      l_time_clock_ -= std::make_tuple(l_begin + l_deduction.first, l_begin + l_deduction.second);
  }
  l_time_clock_.cut_interval(l_begin_time, l_end_time);
  return l_time_clock_;
}

// 计算时间
void computing_time_run(
    const chrono::year_month& in_year_month, const business::work_clock2& in_time_clock, const uuid& in_person_id,
    std::vector<computing_time_post_req_data>& in_data, std::vector<work_xlsx_task_info_helper::database_t>& in_out_data
) {
  DOODLE_CHICK(in_data.size() == in_out_data.size(), "in_data.size() != in_out_data.size()");

  auto l_end_time = chrono::local_days{(in_year_month + chrono::months{1}) / chrono::day{1}} - chrono::seconds{1};
  chrono::local_time_pos l_begin_time{chrono::local_days{in_year_month / chrono::day{1}} + chrono::seconds{1}};
  auto l_all_works = in_time_clock(l_begin_time, l_end_time);

  // 进行排序
  std::ranges::sort(in_data, [](auto&& l_left, auto&& l_right) { return l_left.start_time < l_right.start_time; });
  // });

  {
    // 计算时间比例
    std::vector<std::int64_t> l_woeks1{};
    for (auto&& l_task : in_data) {
      l_woeks1.push_back(std::abs(chrono::floor<chrono::days>(l_task.end_time - l_task.start_time).count()) + 1);
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

    for (auto i = 0; i < in_data.size(); ++i) {
      auto l_end = in_time_clock.next_time(l_begin_time, l_woeks2[i]);
      if (i + 1 == in_data.size()) l_end = l_end_time;

      auto l_info                       = in_time_clock.get_time_info(l_begin_time, l_end);
      std::string l_remark              = fmt::format("{}", fmt::join(l_info, ", "));

      in_out_data[i].start_time_        = {chrono::current_zone(), l_begin_time};
      in_out_data[i].end_time_          = {chrono::current_zone(), l_end};
      in_out_data[i].duration_          = in_time_clock(l_begin_time, l_end);
      in_out_data[i].remark_            = l_remark;
      in_out_data[i].year_month_        = chrono::local_days{in_year_month / 1};
      in_out_data[i].person_id_         = in_person_id;
      in_out_data[i].kitsu_task_ref_id_ = in_data[i].task_id;
      l_begin_time                      = l_end;
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
// 平均时间
void average_time_run(
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
    // 平均时间
    std::vector<chrono::seconds> l_work{};
    using rational_int = boost::rational<std::int64_t>;
    for (auto i = 0; i < in_out_data.size(); ++i) {
      l_work.push_back(
          chrono::seconds{boost::rational_cast<std::int64_t>(rational_int{1, in_out_data.size()} * l_all_works.count())}
      );
    }

    chrono::local_time_pos l_begin_time{chrono::local_days{in_year_month / chrono::day{1}} + chrono::seconds{1}};
    for (auto i = 0; i < in_out_data.size(); ++i) {
      auto l_end = in_time_clock.next_time(l_begin_time, l_work[i]);
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

boost::asio::awaitable<boost::beast::http::message_generator> computing_time::post(session_data_ptr in_handle) {
  if (user_id_ != person_.person_.uuid_id_) person_.check_supervisor();

  auto l_json      = in_handle->get_json();

  auto l_data      = l_json.get<std::vector<computing_time_post_req_data>>();
  auto l_user      = g_ctx().get<sqlite_database>().get_by_uuid<person>(user_id_);
  auto l_block_ptr = std::make_shared<std::vector<work_xlsx_task_info_helper::database_t>>();
  auto l_sql       = g_ctx().get<sqlite_database>();
  using namespace sqlite_orm;
  auto l_ids = l_sql.impl_->storage_any_.select(
      &work_xlsx_task_info_helper::database_t::id_,
      where(
          c(&work_xlsx_task_info_helper::database_t::person_id_) == l_user.uuid_id_ &&
          c(&work_xlsx_task_info_helper::database_t::year_month_) == chrono::local_days{year_month_ / 1}
      )
  );
  co_await l_sql.remove<work_xlsx_task_info_helper::database_t>(l_ids);
  {  // 检查除空以外的id是否重复
    std::map<uuid, std::size_t> l_map;
    for (auto&& l_task : l_data) {
      if (l_task.task_id.is_nil())
        co_return in_handle->make_error_code_msg(boost::beast::http::status::bad_request, "task_id 不可为空");
      l_map[l_task.task_id]++;
    }
    if (l_map.contains(uuid{})) l_map.erase(uuid{});
    if (std::ranges::any_of(l_map, [](const auto& p) { return p.second > 1; }))
      co_return in_handle->make_error_code_msg(boost::beast::http::status::bad_request, "提交的task id 有重复");
    std::set<uuid> l_block_ids{};
    for (auto&& l_task : *l_block_ptr) l_block_ids.emplace(l_task.uuid_id_);
    for (auto&& l_task : l_data) {
      if (l_block_ids.contains(l_task.task_id))
        co_return in_handle->make_error_code_msg(
            boost::beast::http::status::bad_request, fmt::format("每个任务每个月只能有一个 {}", l_task.task_id)
        );
    }
  }
  l_block_ptr->resize(l_data.size());
  auto l_time_clock = create_time_clock(year_month_, l_user.uuid_id_);
  computing_time_run(year_month_, l_time_clock, l_user.uuid_id_, l_data, *l_block_ptr);

  co_await g_ctx().get<sqlite_database>().install_range(l_block_ptr);

  co_return in_handle->make_msg(nlohmann::json{} = get_task_fulls(*l_block_ptr));
}
boost::asio::awaitable<boost::beast::http::message_generator> computing_time::get(session_data_ptr in_handle) {
  if (user_id_ != person_.person_.uuid_id_) person_.check_supervisor();
  auto l_logger    = in_handle->logger_;
  auto l_user      = g_ctx().get<sqlite_database>().get_by_uuid<person>(user_id_);
  auto l_block_ptr = std::make_shared<std::vector<work_xlsx_task_info_helper::database_t>>();

  *l_block_ptr =
      g_ctx().get<sqlite_database>().get_work_xlsx_task_info(l_user.uuid_id_, chrono::local_days{year_month_ / 1});
  *l_block_ptr |= ranges::actions::sort;
  co_return in_handle->make_msg(nlohmann::json{} = get_task_fulls(*l_block_ptr));
}
boost::asio::awaitable<boost::beast::http::message_generator> computing_time_add::post(session_data_ptr in_handle) {
  if (user_id_ != person_.person_.uuid_id_) person_.check_supervisor();

  auto l_json                         = in_handle->get_json();
  computing_time_post_req_data l_data = l_json.get<computing_time_post_req_data>();

  auto l_user                         = g_ctx().get<sqlite_database>().get_by_uuid<person>(user_id_);

  {
    work_xlsx_task_info_helper::database_t l_data_work{
        .start_time_ =
            work_xlsx_task_info_helper::database_t::zoned_time{
                chrono::current_zone(),
                chrono::time_point_cast<work_xlsx_task_info_helper::database_t::zoned_time::duration>(l_data.start_time)
            },
        .end_time_ =
            work_xlsx_task_info_helper::database_t::zoned_time{
                chrono::current_zone(),
                chrono::time_point_cast<work_xlsx_task_info_helper::database_t::zoned_time::duration>(l_data.end_time)
            },
        .year_month_        = chrono::local_days{year_month_ / 1},
        .person_id_         = l_user.uuid_id_,
        .kitsu_task_ref_id_ = l_data.task_id,
    };
    chrono::local_time_pos l_end_time =
        chrono::local_days{(year_month_ + chrono::months{1}) / chrono::day{1}} - chrono::seconds{1};

    chrono::local_time_pos l_begin_time{chrono::local_days{year_month_ / chrono::day{1}} + chrono::seconds{1}};

    l_data_work.start_time_ = std::clamp(
        chrono::time_point_cast<chrono::local_time_pos::duration>(l_data_work.start_time_.get_local_time()),
        l_begin_time, l_end_time
    );
    l_data_work.end_time_ = std::clamp(
        chrono::time_point_cast<chrono::local_time_pos::duration>(l_data_work.end_time_.get_local_time()), l_begin_time,
        l_end_time
    );
    co_await g_ctx().get<sqlite_database>().install(
        std::make_shared<work_xlsx_task_info_helper::database_t>(std::move(l_data_work))
    );
  }
  auto l_block_ptr = std::make_shared<std::vector<work_xlsx_task_info_helper::database_t>>();
  *l_block_ptr =
      g_ctx().get<sqlite_database>().get_work_xlsx_task_info(l_user.uuid_id_, chrono::local_days{year_month_ / 1});
  auto l_time_clock = create_time_clock(year_month_, l_user.uuid_id_);
  recomputing_time_run(year_month_, l_time_clock, *l_block_ptr);
  co_await g_ctx().get<sqlite_database>().update_range(l_block_ptr);

  co_return in_handle->make_msg(nlohmann::json{} = get_task_fulls(*l_block_ptr));
}

boost::asio::awaitable<boost::beast::http::message_generator> computing_time_custom::post(session_data_ptr in_handle) {
  if (user_id_ != person_.person_.uuid_id_) person_.check_supervisor();
  auto l_json                                = in_handle->get_json();
  computing_time_post_req_custom_data l_data = l_json.get<computing_time_post_req_custom_data>();

  l_data.user_id_                            = user_id_;
  l_data.year_month_                         = year_month_;

  auto l_user                                = g_ctx().get<sqlite_database>().get_by_uuid<person>(l_data.user_id_);
  co_await g_ctx().get<sqlite_database>().install(
      std::make_shared<work_xlsx_task_info_helper::database_t>(
          work_xlsx_task_info_helper::database_t{
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
                      chrono::time_point_cast<work_xlsx_task_info_helper::database_t::zoned_time::duration>(
                          l_data.end_time
                      )
                  },
              .user_remark_  = l_data.remark,
              .year_month_   = chrono::local_days{l_data.year_month_ / 1},
              .person_id_    = l_user.uuid_id_,
              .season_       = l_data.season,
              .episode_      = l_data.episode,
              .name_         = l_data.name,
              .grade_        = l_data.grade,
              .project_id_   = l_data.project_id,
              .project_name_ = l_data.project_name_
          }

      )

  );

  auto l_block_ptr = std::make_shared<std::vector<work_xlsx_task_info_helper::database_t>>();
  *l_block_ptr     = g_ctx().get<sqlite_database>().get_work_xlsx_task_info(
      l_user.uuid_id_, chrono::local_days{l_data.year_month_ / 1}
  );
  auto l_time_clock = create_time_clock(l_data.year_month_, l_user.uuid_id_);
  recomputing_time_run(l_data.year_month_, l_time_clock, *l_block_ptr);
  co_await g_ctx().get<sqlite_database>().update_range(l_block_ptr);

  co_return in_handle->make_msg(nlohmann::json{} = get_task_fulls(*l_block_ptr));
}

boost::asio::awaitable<boost::beast::http::message_generator> computing_time_sort::post(session_data_ptr in_handle) {
  if (user_id_ != person_.person_.uuid_id_) person_.check_supervisor();
  auto l_json = in_handle->get_json();
  auto l_data = l_json.get<std::vector<uuid>>();
  auto l_user = g_ctx().get<sqlite_database>().get_by_uuid<person>(user_id_);

  const auto l_block =
      g_ctx().get<sqlite_database>().get_work_xlsx_task_info(l_user.uuid_id_, chrono::local_days{year_month_ / 1});

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

  auto l_time_clock = create_time_clock(year_month_, l_user.uuid_id_);
  recomputing_time_run(year_month_, l_time_clock, *l_block_sort);
  co_await g_ctx().get<sqlite_database>().update_range(l_block_sort);
  co_return in_handle->make_msg(nlohmann::json{} = get_task_fulls(*l_block_sort));
}

boost::asio::awaitable<boost::beast::http::message_generator> computing_time_average::post(session_data_ptr in_handle) {
  if (user_id_ != person_.person_.uuid_id_) person_.check_supervisor();
  auto l_block = std::make_shared<std::vector<work_xlsx_task_info_helper::database_t>>();
  *l_block     = g_ctx().get<sqlite_database>().get_work_xlsx_task_info(user_id_, chrono::local_days{year_month_ / 1});

  auto l_time_clock = create_time_clock(year_month_, user_id_);
  average_time_run(year_month_, l_time_clock, *l_block);
  co_await g_ctx().get<sqlite_database>().update_range(l_block);

  co_return in_handle->make_msg(nlohmann::json{} = get_task_fulls(*l_block));
}

boost::asio::awaitable<boost::beast::http::message_generator> computing_time_patch::patch(session_data_ptr in_handle) {
  if (user_id_ != person_.person_.uuid_id_) person_.check_supervisor();
  auto l_json = in_handle->get_json();

  std::optional<chrono::microseconds> l_duration =
      l_json.contains("work_duration")
          ? std::optional{chrono::microseconds{l_json["work_duration"].get<std::int64_t>()}}
          : std::nullopt;
  std::optional<std::string> l_comment =
      l_json.contains("work_user_remark") ? std::optional{l_json["work_user_remark"].get<std::string>()} : std::nullopt;
  std::optional<std::int32_t> l_eps = l_json.contains("entity_ji_shu_lie")
                                          ? std::optional{l_json["entity_ji_shu_lie"].get<std::int32_t>()}
                                          : std::nullopt;

  if (l_duration && l_duration->count() <= 0) {
    co_return in_handle->make_error_code_msg(
        boost::beast::http::status::bad_request,
        boost::system::error_code{boost::system::errc::bad_message, boost::system::generic_category()}, "参数错误"
    );
  }

  auto l_user      = g_ctx().get<sqlite_database>().get_by_uuid<person>(user_id_);
  auto l_block_ptr = std::make_shared<std::vector<work_xlsx_task_info_helper::database_t>>();
  *l_block_ptr =
      g_ctx().get<sqlite_database>().get_work_xlsx_task_info(l_user.uuid_id_, chrono::local_days{year_month_ / 1});

  if (l_block_ptr->empty()) {
    auto l_year_month_str_1 =
        fmt::format("{}-{}", std::int32_t{year_month_.year()}, std::uint32_t{year_month_.month()});

    co_return in_handle->make_error_code_msg(
        boost::beast::http::status::not_found,
        boost::system::error_code{boost::system::errc::bad_message, boost::system::generic_category()},
        fmt::format("找不到用户 {} 中 {} 月 对应的表格", l_user.phone_, l_year_month_str_1)
    );
  }
  if (l_duration) {
    auto l_timer_clock = create_time_clock(year_month_, l_user.uuid_id_);
    if (auto l_err = patch_time(l_timer_clock, *l_block_ptr, task_id_, *l_duration, in_handle->logger_);
        l_err.empty()) {
      co_await g_ctx().get<sqlite_database>().update_range(l_block_ptr);
    } else {
      co_return in_handle->make_error_code_msg(boost::beast::http::status::bad_request, l_err);
    }
  } else if (l_comment) {
    auto l_block_ptr_value = std::make_shared<work_xlsx_task_info_helper::database_t>();
    for (auto&& l_b : *l_block_ptr) {
      if (l_b.uuid_id_ == task_id_) {
        l_b.user_remark_   = *l_comment;
        *l_block_ptr_value = l_b;
        break;
      }
    }
    co_await g_ctx().get<sqlite_database>().update(l_block_ptr_value);
  } else if (l_eps) {
    auto l_block_ptr_value = std::make_shared<work_xlsx_task_info_helper::database_t>();
    for (auto&& l_b : *l_block_ptr) {
      if (l_b.uuid_id_ == task_id_) {
        l_b.episode_       = *l_eps;
        *l_block_ptr_value = l_b;
        break;
      }
    }
    co_await g_ctx().get<sqlite_database>().update(l_block_ptr_value);
  }
  co_return in_handle->make_msg(nlohmann::json{} = get_task_fulls(*l_block_ptr));
}

boost::asio::awaitable<boost::beast::http::message_generator> computing_time_delete::delete_(
    session_data_ptr in_handle
) {
  work_xlsx_task_info_helper::database_t l_task =
      g_ctx().get<sqlite_database>().get_by_uuid<work_xlsx_task_info_helper::database_t>(id_);
  if (l_task.person_id_ != person_.person_.uuid_id_) person_.check_supervisor();

  SPDLOG_LOGGER_WARN(g_logger_ctrl().get_http(), "delete task id {} user id {}", l_task.uuid_id_, l_task.person_id_);
  co_await g_ctx().get<sqlite_database>().remove<work_xlsx_task_info_helper::database_t>(l_task.id_);
  chrono::year_month_day l_year_month_day{l_task.year_month_};
  chrono::year_month l_year_month{l_year_month_day.year(), l_year_month_day.month()};

  auto l_block_ptr  = std::make_shared<std::vector<work_xlsx_task_info_helper::database_t>>();
  *l_block_ptr      = g_ctx().get<sqlite_database>().get_work_xlsx_task_info(l_task.person_id_, l_task.year_month_);

  auto l_time_clock = create_time_clock(l_year_month, l_task.person_id_);
  recomputing_time_run(l_year_month, l_time_clock, *l_block_ptr);

  co_await g_ctx().get<sqlite_database>().update_range(l_block_ptr);
  co_return in_handle->make_msg(nlohmann::json{} = get_task_fulls(*l_block_ptr));
}

boost::asio::awaitable<void> recomputing_time(const uuid& in_person_id, const chrono::year_month& in_year_month) {
  auto l_block_ptr = std::make_shared<std::vector<work_xlsx_task_info_helper::database_t>>();
  *l_block_ptr =
      g_ctx().get<sqlite_database>().get_work_xlsx_task_info(in_person_id, chrono::local_days{in_year_month / 1});
  auto l_timer_clock = create_time_clock(in_year_month, in_person_id);
  recomputing_time_run(in_year_month, l_timer_clock, *l_block_ptr);
  co_return co_await g_ctx().get<sqlite_database>().update_range(l_block_ptr);
}

}  // namespace doodle::http
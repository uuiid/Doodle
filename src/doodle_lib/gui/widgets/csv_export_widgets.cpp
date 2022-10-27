//
// Created by TD on 2022/2/17.
//
///@brief 表格导出的界面///
#include "csv_export_widgets.h"

#include <doodle_core/core/core_sig.h>
#include <doodle_core/core/doodle_lib.h>
#include <doodle_core/core/init_register.h>
#include <doodle_core/lib_warp/boost_fmt_rational.h>
#include <doodle_core/metadata/assets.h>
#include <doodle_core/metadata/assets_file.h>
#include <doodle_core/metadata/comment.h>
#include <doodle_core/metadata/episodes.h>
#include <doodle_core/metadata/importance.h>
#include <doodle_core/metadata/project.h>
#include <doodle_core/metadata/season.h>
#include <doodle_core/metadata/shot.h>
#include <doodle_core/metadata/time_point_wrap.h>
#include <doodle_core/metadata/user.h>
#include <doodle_core/time_tool/work_clock.h>

#include <doodle_app/gui/base/ref_base.h>
#include <doodle_app/gui/open_file_dialog.h>
#include <doodle_app/lib_warp/imgui_warp.h>

#include <boost/contract.hpp>

#include <fmt/chrono.h>

namespace doodle {
namespace gui {

namespace csv_export_widgets_ns {
csv_line::csv_line(
    const entt::handle &in_handle, const entt::handle &in_up_time_handle, const entt::handle &in_user_handle,
    bool in_use_first_as_project_name, const std::string_view &in_season_fmt_str,
    const std::string_view &in_episodes_fmt_str, const std::string_view &in_shot_fmt_str
) {
  DOODLE_CHICK(in_handle.any_of<assets_file>(), doodle_error{"缺失文件组件"});
  auto &k_ass       = in_handle.get<assets_file>();
  /// \brief 工作时间计算
  auto &work_clock  = in_user_handle.get<business::work_clock>();
  auto project_root = g_reg()->ctx().at<project>().p_path;
  auto start_time   = in_up_time_handle == in_handle ? in_up_time_handle.get<time_point_wrap>().current_month_start()
                                                     : in_up_time_handle.get<time_point_wrap>();
  auto end_time     = in_handle.get<time_point_wrap>();
  /// \brief 计算持续时间
  auto k_time       = work_clock(start_time, end_time);

  comment k_comm{};
  if (auto l_c = in_handle.try_get<comment>(); l_c) k_comm = *l_c;

  if (auto l_time_com = work_clock.get_time_info(start_time, end_time); l_time_com) {
    k_comm.p_time_info = *l_time_com;
  } else {
    k_comm.p_time_info.clear();
  }

  auto l_prj_name = g_reg()->ctx().at<project>().p_name;
  if (in_use_first_as_project_name)
    l_prj_name = in_handle.all_of<assets>() ? in_handle.get<assets>().p_path.begin()->generic_string() : ""s;
  auto k_ass_path = in_handle.all_of<assets>() ? in_handle.get<assets>().p_path : FSys::path{};
  if (in_use_first_as_project_name && !k_ass_path.empty()) {
    k_ass_path = fmt::to_string(fmt::join(++k_ass_path.begin(), k_ass_path.end(), "/"));
  }

  auto l_season =                                                          //"季数"
      in_handle.all_of<season>()                                           //
          ? fmt::format(in_season_fmt_str, in_handle.get<season>().p_int)  //
          : ""s;

  using days_double   = chrono::duration<std::float_t, std::ratio<60ull * 60ull * 8ull>>;
  using time_rational = boost::rational<std::uint64_t>;
  time_rational l_time_rational{chrono::floor<chrono::seconds>(k_time).count(), 60ull * 60ull * 8ull};

  organization_        = k_ass.organization_attr();
  user_                = in_user_handle.get<user>().get_name();
  project_season_name_ = fmt::format("《{}》 {}", l_prj_name, l_season);
  episodes_ =
      in_handle.all_of<episodes>() ? fmt::format(in_episodes_fmt_str, in_handle.get<episodes>().p_episodes) : ""s;
  shot_         = in_handle.all_of<shot>()
                      ? fmt::format(in_shot_fmt_str, in_handle.get<shot>().p_shot, in_handle.get<shot>().p_shot_enum)
                      : ""s;
  start_time_   = start_time;
  end_time_     = end_time;
  len_time_     = chrono::floor<chrono::seconds>(k_time);
  time_info_    = k_comm.p_time_info;
  comment_info_ = k_comm.get_comment();
  file_path_    = k_ass_path.generic_string();
  name_attr_    = k_ass.name_attr();
  cutoff_attr_  = in_handle.any_of<importance>() ? in_handle.get<importance>().cutoff_p : ""s;
  DOODLE_LOG_INFO("计算时间为 {}", len_time_);
}
void csv_table::computing_time() {
  ranges::for_each(line_list, [&](const csv_export_widgets_ns::csv_line &in_line) {
    time_statistics[in_line.user_] += in_line.len_time_;
  });

  DOODLE_LOG_INFO("计算时间总计 {}", fmt::join(time_statistics, " "));
}
}  // namespace csv_export_widgets_ns

class csv_line_gui {
 public:
  csv_line_gui() = default;

  std::string organization_{};
  std::string user_{};
  std::string project_season_name_{};
  std::string episodes_{};
  std::string shot_{};
  std::string start_time_{};
  std::string end_time_{};
  std::string len_time_{};
  std::string time_info_{};
  std::string comment_info_{};
  std::string file_path_{};
  std::string name_attr_{};
  std::string cutoff_attr_{};
};

class csv_table_gui : public gui_cache<csv_export_widgets::csv_table> {
 public:
  std::vector<csv_line_gui> list;
};

class work_clock_method_gui {
 public:
  gui_cache<std::string> data{"获取工作时间方法"s};
  csv_export_widgets::work_clock_method method{csv_export_widgets::work_clock_method::form_dingding};
};

class csv_export_widgets::impl {
 public:
  impl() = default;
  std::vector<entt::handle> list;
  std::vector<entt::handle> list_sort_time;
  std::map<entt::handle, std::vector<entt::handle>> user_handle;

  std::vector<boost::signals2::scoped_connection> con;

  gui_cache<std::string, gui_cache_path> export_path{"导出路径"s, ""s};
  gui_cache<bool> use_first_as_project_name{"分类作为项目名称", true};
  gui_cache<std::string> season_fmt_str{"季数格式化"s, "第{}季"s};
  gui_cache<std::string> episodes_fmt_str{"集数格式化"s, "EP {}"s};
  gui_cache<std::string> shot_fmt_str{"镜头格式化"s, "sc {}{}"s};
  gui_cache<bool> average_time{"平均时间"s, false};
  std::string title_name_;

  gui_cache_name_id gen_table{"生成表"};
  gui_cache_name_id export_table{"导出表"};
  csv_table_gui csv_table_gui_{};
  work_clock_method_gui work_clock_method_gui_{};
};

csv_export_widgets::csv_export_widgets() : p_i(std::make_unique<impl>()) {
  p_i->title_name_ = std::string{name};  // 获取上一次人物提交时的实体name
}
csv_export_widgets::~csv_export_widgets() = default;  /// 析构释放内存

void csv_export_widgets::init() {
  boost::contract::old_ptr<FSys::path> l_old_ptr = BOOST_CONTRACT_OLDOF(p_i->export_path.path);
  boost::contract::check l_check                 = boost::contract::public_function(this)
                                       .precondition([&]() { BOOST_CONTRACT_CHECK(p_i->con.empty()); })
                                       .old([&]() {

                                       })
                                       .postcondition([&]() {
                                         BOOST_CONTRACT_CHECK(p_i->export_path.path.extension() == ".csv");
                                         BOOST_CONTRACT_CHECK(p_i->con.size() == 1);
                                       });

  g_reg()->ctx().emplace<csv_export_widgets &>(*this);  // 如果没有被列表列进去把指针指向的数据传进列表？？
  if (g_reg()->ctx().contains<std::vector<entt::handle>>()) p_i->list = g_reg()->ctx().at<std::vector<entt::handle>>();
  p_i->con.emplace_back(g_reg()->ctx().at<core_sig>().select_handles.connect([this](const std::vector<entt::handle> &in
                                                                             ) { p_i->list = in; }));
  p_i->export_path.path = FSys::temp_directory_path() / "tset.csv";
  p_i->export_path.data = p_i->export_path.path.generic_string();
}

void csv_export_widgets::render() {
  if (ImGui::InputText(*p_i->export_path.gui_name, &p_i->export_path.data))
    p_i->export_path.path = p_i->export_path.data;
  ImGui::SameLine();
  if (ImGui::Button("选择")) {
    auto l_file = std::make_shared<file_dialog>(file_dialog::dialog_args{}.set_title("选择目录"s).set_use_dir());
    l_file->async_read([this](const FSys::path &in) {
      p_i->export_path.path = in / "tmp.csv";
      p_i->export_path.data = p_i->export_path.path.generic_string();
    });
    make_handle().emplace<gui_windows>(l_file);
  }
  ImGui::Checkbox(*p_i->use_first_as_project_name.gui_name, &p_i->use_first_as_project_name.data);
  ImGui::InputText(*p_i->season_fmt_str.gui_name, &p_i->season_fmt_str.data);
  ImGui::InputText(*p_i->episodes_fmt_str.gui_name, &p_i->episodes_fmt_str.data);
  ImGui::InputText(*p_i->shot_fmt_str.gui_name, &p_i->shot_fmt_str.data);
  if (ImGui::Button(*p_i->gen_table)) {
    generate_table();
  }
  ImGui::SameLine();
  if (ImGui::Button(*p_i->export_table)) {
    if (p_i->list.empty()) {
      DOODLE_LOG_INFO("选择为空, 不导出");
      return;
    }
    export_csv();
  }
}
void csv_export_widgets::export_csv(const std::vector<entt::handle> &in_list, const FSys::path &in_export_file_path) {
  FSys::ofstream l_f{in_export_file_path};
  l_f << fmt::format(
      "{}\n", fmt::join(
                  std::vector<std::string>{
                      "部门"s, "制作人"s, "项目"s, "集数"s, "镜头"s, "开始时间"s, "结束时间"s, "持续时间/day"s,
                      "时间备注"s, "备注"s, "类别"s, "名称"s, "等级"s},
                  ","
              )
  );  /// @brief 标题

  std::vector<entt::handle> l_h{in_list};
  /// 按照 季数 -> 集数 -> 镜头 排序
  l_h |= ranges::actions::stable_sort([](const entt::handle &in_r, const entt::handle &in_l) {
           return (in_r.all_of<season>() && in_l.all_of<season>()) && (in_r.get<season>() > in_l.get<season>());
         }) |
         ranges::actions::stable_sort([](const entt::handle &in_r, const entt::handle &in_l) {
           return (in_r.all_of<episodes>() && in_l.all_of<episodes>()) && (in_r.get<episodes>() > in_l.get<episodes>());
         }) |
         ranges::actions::stable_sort([](const entt::handle &in_r, const entt::handle &in_l) {
           return (in_r.all_of<shot>() && in_l.all_of<shot>()) && (in_r.get<shot>() > in_l.get<shot>());
         });

  for (auto &&h : l_h) {
    l_f << fmt::format("{}\n", to_csv_line(h.get<assets_file>().user_attr(), h));
  }
  DOODLE_LOG_INFO("导入完成表 {}", in_export_file_path);
}

entt::handle csv_export_widgets::get_user_up_time(const entt::handle &in_user, const entt::handle &in_handle) {
  auto &&l_time_list = p_i->user_handle[in_user];

  auto l_it          = ranges::find(l_time_list, in_handle);
  if (l_it == l_time_list.begin()) {
    return in_handle;
  } else {
    return *(--l_it);
  }
}
const std::string &csv_export_widgets::title() const { return p_i->title_name_; }
void csv_export_widgets::generate_table() {
  p_i->list =
      p_i->list |
      ranges::views::filter([](const entt::handle &in_h) { return in_h.all_of<time_point_wrap, assets_file>(); }) |
      ranges::to_vector;
  p_i->list_sort_time =
      ranges::copy(p_i->list) | ranges::actions::sort([](const entt::handle &in_r, const entt::handle &in_l) -> bool {
        return in_r.get<time_point_wrap>() < in_l.get<time_point_wrap>();
      });
  p_i->list |= ranges::actions::stable_sort([](const entt::handle &in_r, const entt::handle &in_l) -> bool {
    return in_r.get<assets_file>().user_attr().get<user>() < in_l.get<assets_file>().user_attr().get<user>();
  });
  p_i->user_handle.clear();
  /// \brief 这里设置一下时钟规则
  for (auto &&l_u : p_i->list_sort_time) {
    auto l_user_h = l_u.get<assets_file>().user_attr();
    /// \brief 收集用户的配置
    p_i->user_handle[l_u.get<assets_file>().user_attr()].emplace_back(l_u);
    if (!l_user_h.all_of<business::rules, business::work_clock>()) {
      auto &l_ru   = l_user_h.get_or_emplace<business::rules>(business::rules::get_default());
      auto l_tmp_u = business::rules::get_default();
      if (l_ru.work_time().empty()) {
        l_ru.work_time() = l_tmp_u.work_time();
        if (l_ru.work_weekdays().none()) {
          l_ru.work_weekdays(l_tmp_u.work_weekdays());
        }
      }
      if (l_ru.extra_work().empty() && l_ru.work_weekdays().none()) {
        l_ru.work_weekdays(l_tmp_u.work_weekdays());
      }
      auto &l_work_clock = l_user_h.get_or_emplace<business::work_clock>();
      l_work_clock.set_rules(l_ru);
      l_work_clock.set_interval(
          p_i->list_sort_time.front().get<time_point_wrap>().current_month_start(),
          p_i->list_sort_time.back().get<time_point_wrap>().current_month_end()
      );
      DOODLE_LOG_INFO("用户 {} 时间规则 {}", l_user_h.get<user>().get_name(), l_work_clock.debug_print());
    }
  }

  p_i->csv_table_gui_.data.line_list = p_i->list |
                                       ranges::view::transform([this](const entt::handle &in_handle) -> csv_line {
                                         auto l_user = in_handle.get<assets_file>().user_attr();
                                         return csv_line{
                                             in_handle,
                                             get_user_up_time(l_user, in_handle),
                                             l_user,
                                             p_i->use_first_as_project_name.data(),
                                             p_i->season_fmt_str(),
                                             p_i->episodes_fmt_str(),
                                             p_i->shot_fmt_str()};
                                       }) |
                                       ranges::to_vector;
}
void csv_export_widgets::export_csv() {}
void csv_export_widgets::get_work_time() {}

}  // namespace gui
}  // namespace doodle

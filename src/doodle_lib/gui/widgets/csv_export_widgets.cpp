//
// Created by TD on 2022/2/17.
//

#include "csv_export_widgets.h"
#include <doodle_core/core/core_sig.h>
#include <doodle_core/core/doodle_lib.h>
#include <doodle_core/metadata/project.h>
#include <doodle_core/metadata/assets.h>
#include <doodle_core/metadata/assets_file.h>
#include <doodle_core/metadata/season.h>
#include <doodle_core/metadata/episodes.h>
#include <doodle_core/metadata/shot.h>
#include <doodle_core/metadata/user.h>
#include <doodle_core/metadata/time_point_wrap.h>
#include <doodle_core/metadata/comment.h>
#include <doodle_core/metadata/importance.h>
#include <boost/contract.hpp>

#include <doodle_app/lib_warp/imgui_warp.h>
#include <gui/gui_ref/ref_base.h>
#include <doodle_app/gui/open_file_dialog.h>
#include <doodle_core/core/init_register.h>
#include <doodle_core/time_tool/work_clock.h>
#include <doodle_core/lib_warp/boost_fmt_rational.h>

#include <fmt/chrono.h>

namespace doodle {
namespace gui {

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
};

csv_export_widgets::csv_export_widgets()
    : p_i(std::make_unique<impl>()) {
  p_i->title_name_ = std::string{name};
}
csv_export_widgets::~csv_export_widgets() = default;

void csv_export_widgets::init() {
  boost::contract::old_ptr<FSys::path> l_old_ptr = BOOST_CONTRACT_OLDOF(p_i->export_path.path);
  boost::contract::check l_check                 = boost::contract::public_function(this)
                                       .precondition([&]() {
                                         BOOST_CONTRACT_CHECK(p_i->con.empty());
                                       })
                                       .old([&]() {

                                       })
                                       .postcondition([&]() {
                                         BOOST_CONTRACT_CHECK(p_i->export_path.path.extension() == ".csv");
                                         BOOST_CONTRACT_CHECK(p_i->con.size() == 1);
                                       });

  g_reg()->ctx().emplace<csv_export_widgets &>(*this);
  if (g_reg()->ctx().contains<std::vector<entt::handle>>())
    p_i->list = g_reg()->ctx().at<std::vector<entt::handle>>();
  p_i->con.emplace_back(
      g_reg()->ctx().at<core_sig>().select_handles.connect(
          [this](const std::vector<entt::handle> &in) {
            p_i->list = in;
          }
      )
  );
  p_i->export_path.path = FSys::temp_directory_path() / "tset.csv";
  p_i->export_path.data = p_i->export_path.path.generic_string();
}

void csv_export_widgets::render() {
  if (ImGui::InputText(*p_i->export_path.gui_name, &p_i->export_path.data))
    p_i->export_path.path = p_i->export_path.data;
  ImGui::SameLine();
  if (ImGui::Button("选择")) {
    auto l_file = std::make_shared<file_dialog>(file_dialog::dialog_args{}
                                                    .set_title("选择目录"s)
                                                    .set_use_dir());
    l_file->async_read(
        [this](const FSys::path &in) {
          p_i->export_path.path = in / "tmp.csv";
          p_i->export_path.data = p_i->export_path.path.generic_string();
        }
    );
    make_handle().emplace<gui_windows>(l_file);
  }
  ImGui::Checkbox(*p_i->use_first_as_project_name.gui_name, &p_i->use_first_as_project_name.data);
  ImGui::InputText(*p_i->season_fmt_str.gui_name, &p_i->season_fmt_str.data);
  ImGui::InputText(*p_i->episodes_fmt_str.gui_name, &p_i->episodes_fmt_str.data);
  ImGui::InputText(*p_i->shot_fmt_str.gui_name, &p_i->shot_fmt_str.data);

  if (ImGui::Button("导出")) {
    p_i->list = p_i->list |
                ranges::views::filter([](const entt::handle &in_h) {
                  return in_h.all_of<time_point_wrap, assets_file>();
                }) |
                ranges::to_vector;
    p_i->list_sort_time = ranges::copy(p_i->list) |
                          ranges::actions::sort(
                              [](const entt::handle &in_r, const entt::handle &in_l) -> bool {
                                return in_r.get<time_point_wrap>() < in_l.get<time_point_wrap>();
                              }
                          );
    p_i->list |= ranges::actions::stable_sort(
        [](const entt::handle &in_r, const entt::handle &in_l) -> bool {
          return in_r.get<assets_file>().user_attr().get<user>() < in_l.get<assets_file>().user_attr().get<user>();
        }
    );

    //    auto l_user_vector = p_i->list |
    //                         ranges::views::transform([](const entt::handle &in) -> user {
    //                           return in.get<assets_file>().user_attr().get<user>();
    //                         }) |
    //                         ranges::to_vector;
    //    l_user_vector |= ranges::actions::unique;
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

    if (p_i->list.empty()) {
      DOODLE_LOG_INFO("选择为空, 不导出");
      return;
    }

    this->export_csv(p_i->list, p_i->export_path.path);
  }
}
void csv_export_widgets::export_csv(const std::vector<entt::handle> &in_list, const FSys::path &in_export_file_path) {
  FSys::ofstream l_f{in_export_file_path};
  l_f << fmt::format(
      "{}\n",
      fmt::join(
          std::vector<std::string>{
              "部门"s,
              "制作人"s,
              "项目"s,
              "集数"s,
              "镜头"s,
              "开始时间"s,
              "结束时间"s,
              "持续时间/day"s,
              "时间备注"s,
              "备注"s,
              "类别"s,
              "名称"s,
              "等级"s},
          ","
      )
  );  /// @brief 标题

  std::vector<entt::handle> l_h{in_list};
  /// 按照 季数 -> 集数 -> 镜头 排序
  l_h |= ranges::actions::stable_sort([](const entt::handle &in_r, const entt::handle &in_l) {
           return (in_r.all_of<season>() && in_l.all_of<season>()) &&
                  (in_r.get<season>() > in_l.get<season>());
         }) |
         ranges::actions::stable_sort([](const entt::handle &in_r, const entt::handle &in_l) {
           return (in_r.all_of<episodes>() && in_l.all_of<episodes>()) &&
                  (in_r.get<episodes>() > in_l.get<episodes>());
         }) |
         ranges::actions::stable_sort([](const entt::handle &in_r, const entt::handle &in_l) {
           return (in_r.all_of<shot>() && in_l.all_of<shot>()) &&
                  (in_r.get<shot>() > in_l.get<shot>());
         });

  for (auto &&h : l_h) {
    l_f << fmt::format("{}\n", to_csv_line(h.get<assets_file>().user_attr(), h));
  }
  DOODLE_LOG_INFO("导入完成表 {}", in_export_file_path);
}
csv_export_widgets::csv_line csv_export_widgets::to_csv_line(
    const entt::handle &in_user,
    const entt::handle &in
) {
  DOODLE_CHICK(in.any_of<assets_file>(), doodle_error{"缺失文件组件"});
  auto &k_ass       = in.get<assets_file>();
  /// \brief 工作时间计算
  auto &work_clock  = in_user.get<business::work_clock>();
  auto project_root = g_reg()->ctx().at<project>().p_path;
  auto start_time   = get_user_up_time(in_user, in);
  auto end_time     = in.get<time_point_wrap>();
  /// \brief 计算持续时间
  auto k_time       = work_clock(start_time, end_time);

  comment k_comm{};
  if (auto l_c = in.try_get<comment>(); l_c)
    k_comm = *l_c;

  if (auto l_time_com = work_clock.get_time_info(start_time, end_time);
      l_time_com) {
    k_comm.p_time_info = *l_time_com;
  } else {
    k_comm.p_time_info.clear();
  }

  auto l_prj_name = g_reg()->ctx().at<project>().p_name;
  if (p_i->use_first_as_project_name.data)
    l_prj_name = in.all_of<assets>() ? in.get<assets>().p_path.begin()->generic_string() : ""s;
  auto k_ass_path = in.all_of<assets>() ? in.get<assets>().p_path : FSys::path{};
  if (p_i->use_first_as_project_name.data && !k_ass_path.empty()) {
    k_ass_path = fmt::to_string(fmt::join(++k_ass_path.begin(), k_ass_path.end(), "/"));
  }

  auto l_season =                                                          //"季数"
      in.all_of<season>()                                                  //
          ? fmt::format(p_i->season_fmt_str.data, in.get<season>().p_int)  //
          : ""s;

  using days_double   = chrono::duration<std::float_t, std::ratio<60ull * 60ull * 8ull>>;
  using time_rational = boost::rational<std::uint64_t>;
  time_rational l_time_rational{chrono::floor<chrono::seconds>(k_time).count(), 60ull * 60ull * 8ull};
  csv_line l_line{
      k_ass.organization_attr(),                                                                     //"部门"
      in_user.get<user>().get_name(),                                                                //"制作人"
      fmt::format("《{}》 {}", l_prj_name, l_season),                                                //"项目"
      (in.all_of<episodes>()                                                                         //
           ? fmt::format(p_i->episodes_fmt_str.data, in.get<episodes>().p_episodes)                  //
           : ""s),                                                                                   //"集数"
      (in.all_of<shot>()                                                                             //
           ? fmt::format(p_i->shot_fmt_str.data, in.get<shot>().p_shot, in.get<shot>().p_shot_enum)  //
           : ""s),                                                                                   //"镜头"
      fmt::format(R"("{:%Y/%m/%d %H:%M:%S}")", start_time),                                          //"开始时间"
      fmt::format(R"("{:%Y/%m/%d %H:%M:%S}")", end_time),                                            //"结束时间"
      fmt::format("{}", boost::rational_cast<std::double_t>(l_time_rational)),                       //"持续时间"
      fmt::format("{}", k_comm.p_time_info),                                                         //"时间备注"
      fmt::format("{}", k_comm.get_comment()),                                                       //"备注"
      k_ass_path.generic_string(),                                                                   //"类别"
      k_ass.name_attr(),                                                                             //"名称"
      in.any_of<importance>() ? in.get<importance>().cutoff_p : ""s                                  //"等级"
  };

  return l_line;
}

time_point_wrap csv_export_widgets::get_user_up_time(const entt::handle &in_user, const entt::handle &in_handle) {
  auto &&l_time_list = p_i->user_handle[in_user];

  auto l_it          = ranges::find(l_time_list, in_handle);
  if (l_it == l_time_list.begin()) {
    return in_handle.get<time_point_wrap>().current_month_start();
  } else {
    return (--l_it)->get<time_point_wrap>();
  }
}
const std::string &csv_export_widgets::title() const {
  return p_i->title_name_;
}

}  // namespace gui
}  // namespace doodle

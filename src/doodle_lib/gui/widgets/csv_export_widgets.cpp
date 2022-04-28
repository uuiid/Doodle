//
// Created by TD on 2022/2/17.
//

#include "csv_export_widgets.h"
#include <doodle_core/core/core_sig.h>
#include <core/doodle_lib.h>
#include <metadata/project.h>
#include <metadata/assets.h>
#include <metadata/assets_file.h>
#include <metadata/season.h>
#include <metadata/episodes.h>
#include <metadata/shot.h>
#include <metadata/user.h>
#include <metadata/time_point_wrap.h>
#include <metadata/comment.h>
#include <metadata/importance.h>
#include <boost/contract.hpp>

#include <lib_warp/imgui_warp.h>
#include <gui/gui_ref/ref_base.h>
#include <gui/open_file_dialog.h>
#include <doodle_lib/core/init_register.h>
#include <doodle_lib/core/work_clock.h>

#include <fmt/chrono.h>

namespace doodle {
namespace gui {

class csv_export_widgets::impl {
 public:
  impl() = default;
  std::vector<entt::handle> list;
  std::vector<entt::handle> list_sort_time;
  std::map<entt::handle, time_point_wrap> time_map;
  std::map<std::string, std::vector<entt::handle>> user_map;

  std::vector<boost::signals2::scoped_connection> con;

  gui_cache<std::string, gui_cache_path> export_path{"导出路径"s, ""s};
  gui_cache<bool> use_first_as_project_name{"分类作为项目名称", true};
  gui_cache<std::string> season_fmt_str{"季数格式化"s, "第 {} 季"s};
  gui_cache<std::string> episodes_fmt_str{"集数格式化"s, "ep {}"s};
  gui_cache<std::string> shot_fmt_str{"镜头格式化"s, "sc {}{}"s};
  gui_cache<bool> average_time{"平均时间"s, false};
};

csv_export_widgets::csv_export_widgets()
    : p_i(std::make_unique<impl>()) {
  title_name_ = std::string{name};
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
          }));
  p_i->export_path.path = FSys::temp_directory_path() / "tset.csv";
  p_i->export_path.data = p_i->export_path.path.generic_string();
}

void csv_export_widgets::failed() {
}

void csv_export_widgets::render() {
  if (ImGui::InputText(*p_i->export_path.gui_name, &p_i->export_path.data))
    p_i->export_path.path = p_i->export_path.data;
  ImGui::SameLine();
  if (ImGui::Button("选择")) {
    auto l_file = std::make_shared<FSys::path>();
    g_main_loop()
        .attach<file_dialog>(file_dialog::dialog_args{l_file}
                                 .set_title("选择目录"s)
                                 .set_use_dir())
        .then<one_process_t>([=]() {
          p_i->export_path.path = *l_file / "tmp.csv";
          p_i->export_path.data = p_i->export_path.path.generic_string();
        });
  }
  ImGui::Checkbox(*p_i->use_first_as_project_name.gui_name, &p_i->use_first_as_project_name.data);
  ImGui::Checkbox(*p_i->average_time.gui_name, &p_i->average_time.data);
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
                              });
    p_i->list |= ranges::actions::stable_sort(
        [](const entt::handle &in_r, const entt::handle &in_l) -> bool {
          return in_r.get<assets_file>().p_user < in_l.get<assets_file>().p_user;
        });
    if (p_i->list.empty()) {
      DOODLE_LOG_INFO("选择为空, 不导出");
      return;
    }

    p_i->time_map = p_i->list_sort_time |
                    ranges::view::transform([](const entt::handle &in_handle) -> std::pair<entt::handle, time_point_wrap> {
                      return std::make_pair(in_handle, in_handle.get<time_point_wrap>());
                    }) |
                    ranges::to<std::map<entt::handle, time_point_wrap>>();
    p_i->user_map.clear();
    if (p_i->average_time.data) {  /// \brief 如果需要平均时间, 现在我们就要平均一下
      /// \brief 获取人物名称列表
      ranges::for_each(p_i->list_sort_time, [&](const entt::handle &in_handle) {
        p_i->user_map[in_handle.get<assets_file>().p_user].push_back(in_handle);
      });

      /// \brief 获取大小
      /// \brief 开始平均
      for (auto &&i : p_i->user_map) {
        auto l_beg = time_point_wrap::current_month_start(i.second.front().get<time_point_wrap>()).zoned_time_.get_local_time();
        auto l_end = time_point_wrap::current_month_end(i.second.back().get<time_point_wrap>()).zoned_time_.get_local_time();
        DOODLE_LOG_INFO("获取开始时间 {}, 结束时间 {}", l_beg, l_end);

        auto l_size = i.second.size();
        auto l_time = doodle::work_duration(
                          l_beg,
                          l_end,
                          doodle::business::rules{}) /
                      l_size;

        for (auto j = 0; j < l_size; ++j) {
          auto l_t                   = doodle::next_time(l_beg, l_time * (j + 1), doodle::business::rules{});
          p_i->time_map[i.second[j]] = time_point_wrap{l_t};
          DOODLE_LOG_DEBUG("平均时间 {}", l_t);
        }
      }
    }
    this->export_csv(p_i->list, p_i->export_path.path);
  }
}
void csv_export_widgets::export_csv(const std::vector<entt::handle> &in_list,
                                    const FSys::path &in_export_file_path) {
  FSys::ofstream l_f{in_export_file_path};
  static const table_line l_tile{
      "部门"s,
      "制作人"s,
      "项目"s,
      "季数"s,
      "集数"s,
      "镜头"s,
      "开始时间"s,
      "结束时间"s,
      "持续时间/h"s,
      "备注"s,
      "类别"s,
      "名称"s,
      "等级"s};
  l_f << fmt::format("{}\n", fmt::join(l_tile, ","));  /// @brief 标题

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
    l_f << fmt::format("{}\n", fmt::join(to_csv_line(h), ","));
  }
  DOODLE_LOG_INFO("导入完成表 {}", in_export_file_path);
}
csv_export_widgets::table_line csv_export_widgets::to_csv_line(const entt::handle &in) {
  chick_true<doodle_error>(in.any_of<assets_file>(), DOODLE_LOC, "缺失文件组件");
  auto &k_ass       = in.get<assets_file>();
  auto project_root = g_reg()->ctx().at<project>().p_path;
  auto start_time   = get_user_up_time(in);
  /// \brief 将时间转换为我们使用的调整时间
  auto end_time     = p_i->time_map[in];  // in.get<time_point_wrap>();
  auto k_time       = start_time.work_duration(end_time);

  comment k_comm{};
  if (auto l_c = in.try_get<comment>(); l_c)
    k_comm = *l_c;

  auto l_prj_name = g_reg()->ctx().at<project>().p_name;
  if (p_i->use_first_as_project_name.data)
    l_prj_name = in.all_of<assets>() ? in.get<assets>().p_path.begin()->generic_string() : ""s;
  auto k_ass_path = in.all_of<assets>() ? in.get<assets>().p_path : FSys::path{};
  if (p_i->use_first_as_project_name.data && !k_ass_path.empty()) {
    k_ass_path = fmt::to_string(fmt::join(++k_ass_path.begin(), k_ass_path.end(), "/"));
  }

  table_line l_line{
      k_ass.organization_p,                                                                          //"部门"
      k_ass.p_user,                                                                                  //"制作人"
      l_prj_name,                                                                                    //"项目"
      (in.all_of<season>()                                                                           //
           ? fmt::format(p_i->season_fmt_str.data, in.get<season>().p_int)                           //
           : ""s),                                                                                   //"季数"
      (in.all_of<episodes>()                                                                         //
           ? fmt::format(p_i->episodes_fmt_str.data, in.get<episodes>().p_episodes)                  //
           : ""s),                                                                                   //"集数"
      (in.all_of<shot>()                                                                             //
           ? fmt::format(p_i->shot_fmt_str.data, in.get<shot>().p_shot, in.get<shot>().p_shot_enum)  //
           : ""s),                                                                                   //"镜头"
      fmt::format(R"("{}")", start_time.show_str()),                                                 //"开始时间"
      fmt::format(R"("{}")", end_time.show_str()),                                                   //"结束时间"
      fmt::format("{:3f}", k_time.count()),                                                          //"持续时间"
      fmt::format("{}", k_comm.get_comment()),                                                       //"备注"
      k_ass_path.generic_string(),                                                                   //"类别"
      k_ass.p_name,                                                                                  //"名称"
      in.any_of<importance>() ? in.get<importance>().cutoff_p : ""s                                  //"等级"
  };

  return l_line;
}

time_point_wrap csv_export_widgets::get_user_up_time(const entt::handle &in_handle) {
  time_point_wrap l_r{};

  auto l_it = ranges::find(p_i->list_sort_time, in_handle);
  if (l_it == p_i->list_sort_time.begin()) {
    return time_point_wrap::current_month_start(in_handle.get<time_point_wrap>());
  } else {
    auto l_dis  = std::distance(l_it, p_i->list_sort_time.end());
    auto end_it = ranges::find_if(
        ranges::make_subrange(p_i->list_sort_time.rbegin() + l_dis,
                              p_i->list_sort_time.rend()),
        [&](const entt::handle &in_l) {
          return in_l.get<assets_file>().p_user == in_handle.get<assets_file>().p_user;
        });

    return end_it == p_i->list_sort_time.rend()
               ? time_point_wrap::current_month_start(in_handle.get<time_point_wrap>())
               : p_i->time_map[*end_it];  /// \brief 这里也是转换为我们调整过的时间
  }
}

}  // namespace gui
}  // namespace doodle

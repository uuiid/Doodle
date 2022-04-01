//
// Created by TD on 2022/2/17.
//

#include "csv_export_widgets.h"
#include <core/core_sig.h>
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
#include <fmt/chrono.h>

namespace doodle {
namespace gui {

class csv_export_widgets::impl {
 public:
  impl()
      : list(),
        list_sort_time(),
        con(),
        export_path("导出路径"s, ""s),
        use_first_as_project_name("分类作为项目名称", true),
        season_fmt_str("季数格式化"s, "第 {} 季"s),
        episodes_fmt_str("集数格式化"s, "ep {}"s),
        shot_fmt_str("镜头格式化"s, "sc {}{}"s) {}
  std::vector<entt::handle> list;
  std::vector<entt::handle> list_sort_time;
  std::vector<boost::signals2::scoped_connection> con;

  gui_cache<std::string, gui_cache_path> export_path;
  gui_cache<bool> use_first_as_project_name;
  gui_cache<std::string> season_fmt_str;
  gui_cache<std::string> episodes_fmt_str;
  gui_cache<std::string> shot_fmt_str;
};

csv_export_widgets::csv_export_widgets()
    : p_i(std::make_unique<impl>()) {
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

  g_reg()->set<csv_export_widgets &>(*this);
  if (auto l_l = g_reg()->try_ctx<std::vector<entt::handle>>(); l_l)
    p_i->list = *l_l;
  p_i->con.emplace_back(
      g_reg()->ctx<core_sig>().select_handles.connect(
          [this](const std::vector<entt::handle> &in) {
            p_i->list = in;
          }));
  p_i->export_path.path = FSys::temp_directory_path() / "tset.csv";
  p_i->export_path.data = p_i->export_path.path.generic_string();
}
void csv_export_widgets::succeeded() {
  g_reg()->unset<csv_export_widgets>();
}
void csv_export_widgets::failed() {
  g_reg()->unset<csv_export_widgets>();
}
void csv_export_widgets::aborted() {
  g_reg()->unset<csv_export_widgets>();
}
void csv_export_widgets::update(const chrono::duration<
                                    chrono::system_clock::rep,
                                    chrono::system_clock::period> &,
                                void *data) {
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

  for (auto &&h : in_list) {
    l_f << fmt::format("{}\n", fmt::join(to_csv_line(h), ","));
  }
  DOODLE_LOG_INFO("导入完成表 {}", in_export_file_path);
}
csv_export_widgets::table_line csv_export_widgets::to_csv_line(const entt::handle &in) {
  chick_true<doodle_error>(in.any_of<assets_file>(), DOODLE_LOC, "缺失文件组件");
  auto &k_ass       = in.get<assets_file>();
  auto project_root = g_reg()->ctx<project>().p_path;
  auto start_time   = get_user_up_time(in);
  auto end_time     = in.get<time_point_wrap>();
  auto k_time       = start_time.work_duration(end_time);
  auto l_file_path  = project_root / k_ass.path;

  comment k_comm{};
  if (auto l_c = in.try_get<comment>(); l_c)
    k_comm = *l_c;

  auto l_prj_name = g_reg()->ctx<project>().p_name;
  if (p_i->use_first_as_project_name.data)
    l_prj_name = in.all_of<assets>() ? in.get<assets>().p_path.begin()->generic_string() : ""s;
  auto k_ass_path = in.all_of<assets>() ? in.get<assets>().p_path : FSys::path{};
  if (p_i->use_first_as_project_name.data && !k_ass_path.empty()) {
    k_ass_path = fmt::to_string(fmt::join(++k_ass_path.begin(), k_ass_path.end(), "/"));
  }

  table_line l_line{
      k_ass.organization_p,                                                                            //"部门"
      k_ass.p_user,                                                                                    //"制作人"
      l_prj_name,                                                                                      //"项目"
      (in.all_of<season>()                                                                             //
           ? fmt::format(p_i->season_fmt_str.data, in.get<season>().p_int)                             //
           : ""s),                                                                                     //"季数"
      (in.all_of<episodes>()                                                                           //
           ? fmt::format(p_i->episodes_fmt_str.data, in.get<episodes>().p_episodes)                    //
           : ""s),                                                                                     //"集数"
      (in.all_of<shot>()                                                                               //
           ? fmt::format(p_i->shot_fmt_str.data, in.get<shot>().p_shot, in.get<shot>().get_shot_ab())  //
           : ""s),                                                                                     //"镜头"
      fmt::format(R"("{}")", start_time.show_str()),                                                   //"开始时间"
      fmt::format(R"("{}")", end_time.show_str()),                                                     //"结束时间"
      fmt::format("{:3f}", k_time.count()),                                                            //"持续时间"
      fmt::format("{}", k_comm.get_comment()),                                                         //"备注"
      k_ass_path.generic_string(),                                                                     //"类别"
      k_ass.p_name,                                                                                    //"名称"
      in.any_of<importance>() ? in.get<importance>().cutoff_p : ""s                                    //"等级"
  };

  return l_line;
}

time_point_wrap csv_export_widgets::get_user_up_time(const entt::handle &in_handle) {
  time_point_wrap l_r{};

  auto l_it = boost::range::find(p_i->list_sort_time, in_handle);
  if (l_it == p_i->list_sort_time.begin()) {
    return time_point_wrap::current_month_start(in_handle.get<time_point_wrap>());
  } else {
    auto l_dis  = std::distance(l_it, p_i->list_sort_time.end());
    auto end_it = boost::find_if(
        std::make_pair(p_i->list_sort_time.rbegin() + l_dis,
                       p_i->list_sort_time.rend()),
        [&](const entt::handle &in_l) {
          return in_l.get<assets_file>().p_user == in_handle.get<assets_file>().p_user;
        });

    return end_it == p_i->list_sort_time.rend()
               ? time_point_wrap::current_month_start(in_handle.get<time_point_wrap>())
               : end_it->get<time_point_wrap>();
  }
}
}  // namespace gui
}  // namespace doodle

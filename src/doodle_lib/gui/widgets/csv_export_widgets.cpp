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
        export_path("导出路径"s, ""s) {}
  std::vector<entt::handle> list;
  std::vector<entt::handle> list_sort_time;
  std::vector<boost::signals2::scoped_connection> con;

  gui_cache<std::string, gui_cache_path> export_path;
};

csv_export_widgets::csv_export_widgets()
    : p_i(std::make_unique<impl>()) {
}
csv_export_widgets::~csv_export_widgets() = default;

void csv_export_widgets::init() {
  g_reg()->set<csv_export_widgets &>(*this);
  if (auto l_l = g_reg()->try_ctx<std::vector<entt::handle>>(); l_l)
    p_i->list = *l_l;
  p_i->con.emplace_back(
      g_reg()->ctx<core_sig>().select_handles.connect(
          [this](const std::vector<entt::handle> &in) {
            p_i->list = in;
          }));
  p_i->export_path.path = FSys::temp_directory_path() / "tset.csv";
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
        .attach<file_dialog>(l_file, "选择目录")
        .then<one_process_t>([=]() {
          p_i->export_path.path = *l_file / "tmp.csv";
          p_i->export_path.data = p_i->export_path.path.generic_string();
        });
  }

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
  l_f << "项目"
         ","
         "所属部门"
         ","
         "集数"
         ","
         "镜头"
         ","
         "名称"
         ","
         "制作人"
         ","
         "开始时间"
         ","
         "结束时间"
         ","
         "持续时间/h"
         ","
         "备注"
         ","
         "文件存在"
         ","
         "文件路径"
      << "\n";  /// @brief 标题
  std::vector<entt::handle> l_h{in_list};
  boost::stable_sort(
      boost::stable_sort(
          l_h,
          [](const entt::handle &in_r, const entt::handle &in_l) {
            return (in_r.all_of<episodes>() && in_l.all_of<episodes>()) &&
                   (in_r.get<episodes>() > in_l.get<episodes>());
          }),
      [](const entt::handle &in_r, const entt::handle &in_l) {
        return (in_r.all_of<shot>() && in_l.all_of<shot>()) &&
               (in_r.get<shot>() > in_l.get<shot>());
      });
  for (auto &&h : in_list) {
    l_f << to_csv_line(h) << "\n";
  }
  DOODLE_LOG_INFO("导入完成表 {}", in_export_file_path);
}
std::string csv_export_widgets::to_csv_line(const entt::handle &in) {
  chick_true<doodle_error>(in.any_of<assets_file>(), DOODLE_LOC, "缺失文件组件");
  std::stringstream l_r{};
  auto &k_ass       = in.get<assets_file>();
  auto project_root = g_reg()->ctx<project>().p_path;
  auto end_time     = get_user_up_time(in);
  auto k_time       = in.get<time_point_wrap>().work_duration(end_time);
  auto l_file_path  = project_root / k_ass.path;

  comment k_comm{};
  if (auto l_c = in.try_get<std::vector<comment>>(); l_c)
    k_comm = l_c->empty() ? l_c->front() : comment{};

  l_r << g_reg()->ctx<project>().p_name << ","
      << magic_enum::enum_name(k_ass.p_department) << ","
      << (in.all_of<episodes>() ? fmt::to_string(in.get<episodes>()) : ""s) << ","
      << (in.all_of<shot>() ? fmt::to_string(in.get<shot>()) : ""s) << ","
      << (in.all_of<assets>() ? in.get<assets>().p_path.generic_string() : ""s) << ","
      << k_ass.p_user << ","
      << fmt::format(R"("{}")", in.get<time_point_wrap>().show_str()) << ","
      << fmt::format(R"("{}")", end_time.show_str()) << ","
      << fmt::format("{:3f}", k_time.count()) << ","
      << fmt::format("{}", k_comm.get_comment()) << ","
      << fmt::to_string(FSys::exists(l_file_path)) << ","
      << l_file_path.generic_string();

  return l_r.str();
}

time_point_wrap csv_export_widgets::get_user_up_time(const entt::handle &in_handle) {
  time_point_wrap l_r{};

  auto l_it = boost::range::find(p_i->list_sort_time, in_handle);
  if (l_it == p_i->list_sort_time.begin()) {
    return time_point_wrap::current_month_end(in_handle.get<time_point_wrap>());
  } else {
    auto end_it = boost::find_if(
        std::make_pair(p_i->list_sort_time.begin(),
                       l_it - 1),
        [&](const entt::handle &in_l) {
          return in_l.get<assets_file>().p_user == in_handle.get<assets_file>().p_user;
        });

    return end_it == p_i->list_sort_time.end()
               ? time_point_wrap::current_month_end(in_handle.get<time_point_wrap>())
               : end_it->get<time_point_wrap>();
  }
}
}  // namespace gui
}  // namespace doodle

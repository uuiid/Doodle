//
// Created by TD on 2022/2/17.
//

#include "csv_export_widgets.h"
#include <core/core_sig.h>
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

namespace doodle {
namespace gui {

class csv_export_widgets::impl {
 public:
  std::vector<entt::handle> list;
  std::vector<entt::handle> list_sort_time;
  std::vector<boost::signals2::scoped_connection> con;
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
  if (ImGui::Button("导出")) {
    p_i->list_sort_time =
        ranges::copy(p_i->list) |
        ranges::views::filter([](const entt::handle &in_h) {
          return in_h.all_of<time_point_wrap>();
        }) |
        ranges::actions::sort(
            [](const entt::handle &in_r, const entt::handle &in_l) -> bool {
              return in_r.get<time_point_wrap>() < in_l.get<time_point_wrap>();
            });
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
         "持续时间"
         ","
         "备注"
         ","
         "文件存在"
         ","
         "文件路径";  /// @brief 标题
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
  auto &k_ass = in.get<assets_file>();
  auto l_next = get_user_next_time(in);
  auto k_time = chrono::floor<chrono::days_double>(
      in.get<time_point_wrap>().work_duration(l_next.get<time_point_wrap>()));

  comment k_comm{};
  if (auto l_c = in.try_get<std::vector<comment>>(); l_c)
    k_comm = l_c->empty() ? l_c->front() : comment{};

  l_r << g_reg()->ctx<project>().p_name << ","
      << magic_enum::enum_name(k_ass.p_department) << ","
      << fmt::to_string(in.get<episodes>()) << ","
      << fmt::to_string(in.get<shot>()) << ","
      << in.get<assets>().p_name_show_str << ","
      << k_ass.p_user << ","
      << fmt::format(R"("{}")", in.get<time_point_wrap>().show_str()) << ","
      << fmt::format(R"("{}")", l_next.get<time_point_wrap>().show_str()) << ","
      << fmt::format("{}", k_time.count()) << ","
      << fmt::format("{}", k_comm.get_comment()) << ","
      << fmt::to_string(FSys::exists(k_ass.path)) << ","
      << k_ass.path;

  return l_r.str();
}

entt::handle csv_export_widgets::get_user_next_time(const entt::handle &in_handle) {
  auto end_it = boost::find_if(std::make_pair(boost::range::find(p_i->list_sort_time, in_handle), p_i->list_sort_time.end()),
                               [&](const entt::handle &in_l) {
                                 return in_l.get<assets_file>().p_user == in_handle.get<assets_file>().p_user;
                               });

  return end_it == p_i->list_sort_time.end() ? entt::handle{} : *end_it;
}
}  // namespace gui
}  // namespace doodle

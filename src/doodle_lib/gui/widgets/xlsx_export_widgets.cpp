//
// Created by TD on 2022/2/17.
//
#include "xlsx_export_widgets.h"

#include "doodle_core/logger/logger.h"
#include <doodle_core/core/core_sig.h>
#include <doodle_core/core/doodle_lib.h>
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

#include "doodle_app/gui/base/base_window.h"
#include <doodle_app/gui/base/ref_base.h>
#include <doodle_app/gui/open_file_dialog.h>
#include <doodle_app/gui/show_message.h>
#include <doodle_app/lib_warp/imgui_warp.h>

#include <doodle_lib/attendance/attendance_rule.h>

#include <boost/contract.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/fusion/adapted/struct/adapt_struct.hpp>
#include <boost/fusion/include/adapt_struct.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <date/date.h>
#include <entt/entity/fwd.hpp>
#include <fmt/chrono.h>
#include <fmt/core.h>
#include <imgui.h>
#include <map>
#include <memory>
#include <range/v3/algorithm/for_each.hpp>
#include <string.h>
#include <string>
#include <utility>
#include <vector>
#include <xlnt/xlnt.hpp>

namespace doodle::gui {
class xlsx_line_gui {
 public:
  xlsx_line_gui() = default;

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

class xlsx_line_statistics_gui {
 public:
  xlsx_line_statistics_gui() = default;
  std::string user_name;
  std::string len_time;
};

}  // namespace doodle::gui
BOOST_FUSION_ADAPT_STRUCT(
    ::doodle::gui::xlsx_line_gui, organization_, user_, project_season_name_, episodes_, shot_, start_time_, end_time_,
    len_time_, time_info_, comment_info_, file_path_, name_attr_, cutoff_attr_
);

BOOST_FUSION_ADAPT_STRUCT(::doodle::gui::xlsx_line_statistics_gui, user_name, len_time);

namespace doodle::gui {

namespace xlsx_export_widgets_ns {
xlsx_line::xlsx_line(
    const entt::handle &in_handle, const std::vector<entt::handle> &in_up_time_handle_list,
    const entt::handle &in_user_handle, bool in_use_first_as_project_name, const std::string_view &in_season_fmt_str,
    const std::string_view &in_episodes_fmt_str, const std::string_view &in_shot_fmt_str
) {
  in_user_handle.any_of<user>() ? void() : throw_error(error_enum::component_missing_error, "缺失用户组件"s);
  DOODLE_CHICK(in_handle.any_of<assets_file>(), doodle_error{"缺失文件组件"});
  auto &k_ass            = in_handle.get<assets_file>();
  /// \brief 工作时间计算
  const auto &work_clock = in_user_handle.get<business::work_clock>();
  auto project_root      = g_reg()->ctx().at<project>().p_path;
  /// 寻找上一个用户
  auto start_time        = in_handle.get<time_point_wrap>().current_month_start();
  auto l_it              = ranges::find(in_up_time_handle_list, in_handle);
  if (l_it != in_up_time_handle_list.begin()) {
    start_time = (--l_it)->get<time_point_wrap>();
  }

  auto end_time = (in_up_time_handle_list.back() == in_handle) ? in_handle.get<time_point_wrap>().current_month_end()
                                                               : in_handle.get<time_point_wrap>();
  /// \brief 计算持续时间
  auto k_time   = work_clock(start_time, end_time);

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
  len_time_     = chrono::round<chrono::seconds>(k_time);
  time_info_    = k_comm.p_time_info;
  comment_info_ = k_comm.get_comment();
  file_path_    = k_ass_path.generic_string();
  name_attr_    = k_ass.name_attr();
  cutoff_attr_  = in_handle.any_of<importance>() ? in_handle.get<importance>().cutoff_p : ""s;
  DOODLE_LOG_INFO("计算时间为 {}", len_time_);
}
bool xlsx_line::operator<(const xlsx_line &in_l) const {
  return std::tie(project_season_name_, episodes_, shot_) <
         std::tie(in_l.project_season_name_, in_l.episodes_, in_l.shot_);
}
bool xlsx_line::operator==(const xlsx_line &in_l) const {
  return std::tie(project_season_name_, episodes_, shot_) ==
         std::tie(in_l.project_season_name_, in_l.episodes_, in_l.shot_);
}

void xlsx_table::computing_time() {
  time_statistics.clear();
  ranges::for_each(line_list, [&](const xlsx_export_widgets_ns::xlsx_line &in_line) {
    time_statistics[in_line.user_] += in_line.len_time_;
  });

  DOODLE_LOG_INFO("计算时间总计 {}", fmt::join(time_statistics, " "));
  std::map<std::string, std::vector<std::size_t>> user_index;

  for (std::size_t l = 0; l < line_list.size(); ++l) {
    user_index[line_list[l].user_].emplace_back(l);
  }

  // 这里我们需要进行求整
  for (auto &&item : time_statistics) {
    /// 先使用小时取整
    auto l_hours      = chrono::round<chrono::hours>(item.second);
    /// 再转换为秒
    auto l_second     = chrono::round<chrono::seconds>(l_hours);
    /// 得出误差
    auto l_mean_error = std::max(item.second, l_second) - std::min(item.second, l_second);
    if (l_mean_error == 0s) continue;

    /// 直接选中第一个调整误差
    /// 多了
    if (item.second > l_second) line_list[user_index[item.first].front()].len_time_ -= l_mean_error;
    /// 少了
    else
      line_list[user_index[item.first].front()].len_time_ += l_mean_error;
  }
  /// 重新统计
  time_statistics.clear();
  ranges::for_each(line_list, [&](const xlsx_export_widgets_ns::xlsx_line &in_line) {
    time_statistics[in_line.user_] += in_line.len_time_;
  });
  DOODLE_LOG_INFO("计算时间调整后总计 {}", fmt::join(time_statistics, " "));
}
void xlsx_table::sort_line() { line_list |= ranges::actions::sort; }

std::string xlsx_table::to_str() const {
  std::ostringstream l_str{};
  l_str << fmt::format(
      "{}\n", fmt::join(
                  {"部门"s, "制作人"s, "项目"s, "集数"s, "镜头"s, "开始时间"s, "结束时间"s, "持续时间/day"s,
                   "时间备注"s, "备注"s, "类别"s, "名称"s, "等级"s},
                  ","
              )
  );  /// @brief 标题
  l_str << fmt::format(
      "{}\n", fmt::join(
                  line_list | ranges::views::transform([](const xlsx_line &in_line) -> std::string {
                    // using days_double   = chrono::duration<std::float_t, std::ratio<60ull * 60ull * 8ull>>;
                    using time_rational = boost::rational<std::uint64_t>;
                    time_rational l_time_rational{in_line.len_time_.count(), 60ull * 60ull * 8ull};

                    return fmt::format(
                        "{},"
                        "{},"
                        "{},"
                        "{},"
                        "{},"
                        "{},"
                        "{},"
                        "{},"
                        "{},"
                        "{},"
                        "{},"
                        "{},"
                        "{}",
                        in_line.organization_, in_line.user_, in_line.project_season_name_, in_line.episodes_,
                        in_line.shot_, in_line.start_time_, in_line.end_time_,
                        boost::rational_cast<std::double_t>(l_time_rational), in_line.time_info_, in_line.comment_info_,
                        in_line.file_path_, in_line.name_attr_, in_line.cutoff_attr_
                    );
                  }),
                  "\n"
              )
  );
  return l_str.str();
}
}  // namespace xlsx_export_widgets_ns

class xlsx_table_gui {
 public:
  gui_cache<xlsx_export_widgets::xlsx_table> gui_data{"预览"s};
  gui_cache_name_id show_details{"显示详细"};
  gui_cache_name_id show_statistics{"显示统计"};

  std::variant<std::vector<xlsx_line_gui>, std::vector<xlsx_line_statistics_gui>> list;

  void set_table_data(const std::vector<xlsx_export_widgets::xlsx_line> &in_xlsx_line) {
    gui_data().line_list = in_xlsx_line;
    gui_data().computing_time();
    gui_data().sort_line();
    /// 添加初始化显示
    list = gui_data().time_statistics |
           ranges::views::transform(
               [](const std::pair<std::string, chrono::seconds> &in_line) -> xlsx_line_statistics_gui {
                 using time_rational = boost::rational<std::uint64_t>;
                 time_rational l_time_rational{in_line.second.count(), 60ull * 60ull * 8ull};
                 return xlsx_line_statistics_gui{
                     in_line.first, fmt::to_string(boost::rational_cast<std::double_t>(l_time_rational))};
               }
           ) |
           ranges::to_vector;
  }

  [[nodiscard]] constexpr std::int32_t size() const {
    return std::holds_alternative<std::vector<xlsx_line_gui>>(list) ? 13 : 2;
  }

  void render() {
    if (ImGui::Button(*show_details)) {
      list = gui_data().line_list |
             ranges::views::transform([](const xlsx_export_widgets_ns::xlsx_line &in_line) -> xlsx_line_gui {
               using time_rational = boost::rational<std::uint64_t>;
               time_rational l_time_rational{in_line.len_time_.count(), 60ull * 60ull * 8ull};

               return xlsx_line_gui{
                   in_line.organization_,
                   in_line.user_,
                   in_line.project_season_name_,
                   in_line.episodes_,
                   in_line.shot_,
                   fmt::to_string(in_line.start_time_),
                   fmt::to_string(in_line.end_time_),
                   fmt::to_string(boost::rational_cast<std::double_t>(l_time_rational)),
                   in_line.time_info_,
                   in_line.comment_info_,
                   in_line.file_path_,
                   in_line.name_attr_,
                   in_line.cutoff_attr_};
             }) |
             ranges::to_vector;
    }
    if (ImGui::Button(*show_statistics)) {
      list = gui_data().time_statistics |
             ranges::views::transform(
                 [](const std::pair<std::string, chrono::seconds> &in_line) -> xlsx_line_statistics_gui {
                   using time_rational = boost::rational<std::uint64_t>;
                   time_rational l_time_rational{in_line.second.count(), 60ull * 60ull * 8ull};
                   return xlsx_line_statistics_gui{
                       in_line.first, fmt::to_string(boost::rational_cast<std::double_t>(l_time_rational))};
                 }
             ) |
             ranges::to_vector;
    }

    dear::Table{*gui_data, size()} && [this]() { std::visit(*this, list); };
  }
  void operator()(const std::vector<xlsx_line_gui> &in_line) const {
    ImGui::TableSetupScrollFreeze(0, 1);  // 顶行可见
    ImGui::TableSetupColumn("部门");
    ImGui::TableSetupColumn("用户");
    ImGui::TableSetupColumn("项目和季数");
    ImGui::TableSetupColumn("集数");
    ImGui::TableSetupColumn("镜头");
    ImGui::TableSetupColumn("开始时间");
    ImGui::TableSetupColumn("结束时间");
    ImGui::TableSetupColumn("时间长度");
    ImGui::TableSetupColumn("时间信息");
    ImGui::TableSetupColumn("提交信息");
    ImGui::TableSetupColumn("路径");
    ImGui::TableSetupColumn("名称");
    ImGui::TableSetupColumn("等级");
    ImGui::TableHeadersRow();
    ranges::for_each(in_line, [](const xlsx_line_gui &in_gui) {
      ImGui::TableNextRow();

      boost::fusion::for_each(in_gui, [](const std::string &in_string) {
        ImGui::TableNextColumn();
        dear::Text(in_string);
      });
    });
  }

  void operator()(const std::vector<xlsx_line_statistics_gui> &in_vector) const {
    ImGui::TableSetupScrollFreeze(0, 1);  // 顶行可见
    ImGui::TableSetupColumn("用户");
    ImGui::TableSetupColumn("总时间");
    ImGui::TableHeadersRow();
    ranges::for_each(in_vector, [](const xlsx_line_statistics_gui &in_gui) {
      ImGui::TableNextRow();

      ImGui::TableNextColumn();
      dear::Text(in_gui.user_name);
      ImGui::TableNextColumn();
      dear::Text(in_gui.len_time);
    });
  }
};

class xlsx_export_widgets::impl {
 public:
  class time_cache {
   public:
    time_cache() = default;
    gui_cache<std::array<std::int32_t, 2>> cache{"年,月", 0, 0};
    time_point_wrap time_data{};
  };

  class user_list_cache : public gui_cache<std::string> {
   public:
    user_list_cache() : gui_cache<std::string>("过滤用户"s, "all"s){};
    std::map<std::string, entt::handle> user_list{};
    entt::handle current_user{};
  };

  class user_clock {
   public:
    explicit user_clock(const std::string &in_basic_string, const std::string &in_phone_number, entt::handle in_handle)
        : user_handle_attr(in_handle),
          phone_number(fmt::format("{}电话", in_basic_string), in_phone_number),
          get_chork("获取时钟"s){};
    entt::handle user_handle_attr{};
    gui_cache<std::string> phone_number;
    gui_cache_name_id get_chork{"获取时钟"s};
  };

  struct gui_path : gui_cache_path {
    FSys::path stem{};
  };

  impl() = default;
  std::vector<entt::handle> list;
  std::vector<entt::handle> list_sort_time;
  std::map<entt::handle, std::vector<entt::handle>> user_handle;

  std::vector<boost::signals2::scoped_connection> con;

  gui_cache<std::string, gui_path> export_path{"导出路径"s, ""s};
  gui_cache<bool> use_first_as_project_name{"分类作为项目名称", true};
  gui_cache<std::string> season_fmt_str{"季数格式化"s, "第{}季"s};
  gui_cache<std::string> episodes_fmt_str{"集数格式化"s, "EP {}"s};
  gui_cache<std::string> shot_fmt_str{"镜头格式化"s, "sc {}{}"s};
  std::string title_name_;
  bool open{true};

  gui_cache_name_id gen_table{"生成表"};
  gui_cache_name_id export_table{"导出表"};
  gui_cache_name_id advanced_setting{"高级设置"};

  xlsx_table_gui xlsx_table_gui_{};
  xlsx_line_statistics_gui xlsx_line_statistics_gui_{};

  std::shared_ptr<business::detail::attendance_interface> attendance_ptr{};
  /// 过滤用户
  user_list_cache combox_user_id{};

  /// 过滤年份,月份
  time_cache combox_month{};
  gui_cache_name_id filter{"过滤"};

  std::size_t user_size{};
};

xlsx_export_widgets::xlsx_export_widgets() : p_i(std::make_unique<impl>()) {
  p_i->title_name_ = std::string{name};  // 获取上一次人物提交时的实体name
  init();
}
xlsx_export_widgets::~xlsx_export_widgets() = default;  /// 析构释放内存

void xlsx_export_widgets::init() {
  boost::contract::old_ptr<FSys::path> l_old_ptr = BOOST_CONTRACT_OLDOF(p_i->export_path.path);
  boost::contract::check l_check                 = boost::contract::public_function(this)
                                       .precondition([&]() { BOOST_CONTRACT_CHECK(p_i->con.empty()); })
                                       .old([&]() {

                                       })
                                       .postcondition([&]() {
                                         BOOST_CONTRACT_CHECK(p_i->export_path.path.extension() == ".xlsx");
                                         BOOST_CONTRACT_CHECK(p_i->con.size() == 1);
                                       });

  if (g_reg()->ctx().contains<std::vector<entt::handle>>()) p_i->list = g_reg()->ctx().at<std::vector<entt::handle>>();
  p_i->con.emplace_back(g_reg()->ctx().at<core_sig>().select_handles.connect([this](const std::vector<entt::handle> &in
                                                                             ) { p_i->list = in; }));
  p_i->export_path.path = FSys::temp_directory_path() / "test.xlsx";
  p_i->export_path.stem = p_i->export_path.path.stem();
  p_i->export_path.data = p_i->export_path.path.generic_string();

  gen_user();
  auto &&[l_y, l_m, l_d, l_h, l_mim, l_s] = p_i->combox_month.time_data.compose();
  p_i->combox_month.cache()               = {l_y, l_m};
}

bool xlsx_export_widgets::render() {
  dear::Begin l_win{p_i->title_name_.data(), &p_i->open};
  if (!l_win) return p_i->open;

  if (ImGui::InputText(*p_i->export_path.gui_name, &p_i->export_path.data)) {
    p_i->export_path.path = p_i->export_path.data;
    p_i->export_path.stem = p_i->export_path.path.stem();
  }
  ImGui::SameLine();
  if (ImGui::Button("选择")) {
    g_windows_manage()
        .create_windows<file_dialog>(file_dialog::dialog_args{}.set_title("选择目录"s).set_use_dir())
        ->async_read([this](const FSys::path &in) {
          p_i->export_path.path = in / "tmp.xlsx";
          p_i->export_path.stem = "tmp";
          p_i->export_path.data = p_i->export_path.path.generic_string();
        });
  }

  dear::TreeNode{*p_i->advanced_setting} && [&]() {
    ImGui::Checkbox(*p_i->use_first_as_project_name.gui_name, &p_i->use_first_as_project_name.data);
    ImGui::InputText(*p_i->season_fmt_str.gui_name, &p_i->season_fmt_str.data);
    ImGui::InputText(*p_i->episodes_fmt_str.gui_name, &p_i->episodes_fmt_str.data);
    ImGui::InputText(*p_i->shot_fmt_str.gui_name, &p_i->shot_fmt_str.data);
  };
  ImGui::PushItemWidth(100);

  if (ImGui::InputInt2(*p_i->combox_month.cache, p_i->combox_month.cache().data())) {
    auto &&[l_y, l_m, l_d, l_h, l_mim, l_s] = p_i->combox_month.time_data.compose();
    p_i->combox_month.time_data =
        time_point_wrap{p_i->combox_month.cache()[0], p_i->combox_month.cache()[1], l_d, l_h, l_mim, l_s};
  }
  ImGui::SameLine();
  dear::Combo{*p_i->combox_user_id, p_i->combox_user_id().c_str()} && [this]() {
    gen_user();
    for (auto &&l_u : p_i->combox_user_id.user_list) {
      if (dear::Selectable(l_u.first.c_str())) {
        p_i->combox_user_id()            = l_u.first;
        p_i->combox_user_id.current_user = l_u.second;
      }
    }
  };

  ImGui::PopItemWidth();
  ImGui::SameLine();
  if (ImGui::Button(*p_i->filter)) {
    filter_();
    get_work_time();
  }

  if (ImGui::Button(*p_i->export_table)) {
    export_xlsx();
  }

  // 渲染表格
  p_i->xlsx_table_gui_.render();

  return p_i->open;
}

const std::string &xlsx_export_widgets::title() const { return p_i->title_name_; }
void xlsx_export_widgets::generate_table() {
  auto &l_p = g_reg()->ctx().emplace<process_message>();
  l_p.set_state(l_p.success);
  p_i->list =
      p_i->list |
      ranges::views::filter([](const entt::handle &in_h) { return in_h.all_of<time_point_wrap, assets_file>(); }) |
      ranges::to_vector;
  p_i->list |= ranges::actions::stable_sort([](const entt::handle &in_r, const entt::handle &in_l) -> bool {
    return in_r.get<assets_file>().user_attr().get<user>() < in_l.get<assets_file>().user_attr().get<user>();
  });
  p_i->xlsx_table_gui_.set_table_data(
      p_i->list | ranges::views::transform([this](const entt::handle &in_handle) -> xlsx_line {
        auto l_user = in_handle.get<assets_file>().user_attr();
        return xlsx_line{
            in_handle,
            p_i->user_handle[l_user],
            l_user,
            p_i->use_first_as_project_name(),
            p_i->season_fmt_str(),
            p_i->episodes_fmt_str(),
            p_i->shot_fmt_str()};
      }) |
      ranges::to_vector
  );
}

void xlsx_export_widgets::export_xlsx() {
  if (p_i->list.empty()) {
    g_windows_manage().create_windows<show_message>("过滤后为空, 不导出");
    DOODLE_LOG_INFO("过滤后为空, 不导出");
    return;
  }
  xlnt::workbook wbOut{};

  const static std::vector<std::string> s_xlsx_header{"部门"s,     "制作人"s,   "项目"s,         "集数"s,     "镜头"s,
                                                      "开始时间"s, "结束时间"s, "持续时间/day"s, "时间备注"s, "备注"s,
                                                      "类别"s,     "名称"s,     "等级"s};

  const auto &l_line = p_i->xlsx_table_gui_.gui_data().line_list;
  const auto &l_user = p_i->xlsx_table_gui_.gui_data().time_statistics;
  std::map<std::string, std::vector<std::size_t>> user_map{};

  for (std::size_t i = 0; i < l_line.size(); i++) {
    user_map[l_line[i].user_].emplace_back(i);
  }

  for (auto &&i : user_map) {
    auto wsOut = wbOut.create_sheet(0);
    wsOut.title(i.first);
    for (auto i = 0; i < s_xlsx_header.size(); ++i) {
      wsOut.cell(xlnt::cell_reference(1 + i, 1)).value(s_xlsx_header[i]);
    }
    std::size_t l_index{2};
    for (auto &&j : i.second) {
      const auto &l_row_ = l_line[j];
      auto l_s_t         = l_row_.start_time_.compose();
      auto l_e_t         = l_row_.end_time_.compose();
      boost::rational<std::uint64_t> l_time_rational{l_row_.len_time_.count(), 60ull * 60ull * 8ull};
      wsOut.cell(xlnt::cell_reference(1, l_index)).value(l_row_.organization_);
      wsOut.cell(xlnt::cell_reference(2, l_index)).value(l_row_.user_);
      wsOut.cell(xlnt::cell_reference(3, l_index)).value(l_row_.project_season_name_);
      wsOut.cell(xlnt::cell_reference(4, l_index)).value(l_row_.episodes_);
      wsOut.cell(xlnt::cell_reference(5, l_index)).value(l_row_.shot_);
      wsOut.cell(xlnt::cell_reference(6, l_index))
          .value(xlnt::datetime{l_s_t.year, l_s_t.month, l_s_t.day, l_s_t.hours, l_s_t.minutes, l_s_t.seconds});
      wsOut.cell(xlnt::cell_reference(7, l_index))
          .value(xlnt::datetime{l_e_t.year, l_e_t.month, l_e_t.day, l_e_t.hours, l_e_t.minutes, l_e_t.seconds});
      // wsOut.cell(xlnt::cell_reference(8, l_index)).number_format(xlnt::number_format::number_00());
      wsOut.cell(xlnt::cell_reference(8, l_index)).value(boost::rational_cast<std::double_t>(l_time_rational));
      wsOut.cell(xlnt::cell_reference(9, l_index)).value(l_row_.time_info_);
      wsOut.cell(xlnt::cell_reference(10, l_index)).value(l_row_.comment_info_);
      wsOut.cell(xlnt::cell_reference(11, l_index)).value(l_row_.file_path_);
      wsOut.cell(xlnt::cell_reference(12, l_index)).value(l_row_.name_attr_);
      wsOut.cell(xlnt::cell_reference(13, l_index)).value(l_row_.cutoff_attr_);
      ++l_index;
    }
  }

  /// 导出表格

  const FSys::path l_p{p_i->export_path.path.parent_path()};
  auto path = p_i->export_path.path;
  for (auto i = 1; i < 100; ++i) {
    FSys::ofstream l_f{path, FSys::ofstream::binary};
    if (l_f.fail()) {
      path = l_p / fmt::format("{}_{}.xlsx", p_i->export_path.stem, i);
      continue;
    }
    p_i->export_path.path = path;
    p_i->export_path.data = path.generic_string();
    wbOut.save(l_f);
    g_windows_manage().create_windows<show_message>(fmt::format("成功导出到路径{}", p_i->export_path.data));
    break;
  }
}
bool xlsx_export_widgets::get_work_time() {
  p_i->list_sort_time =
      ranges::copy(p_i->list) | ranges::actions::sort([](const entt::handle &in_r, const entt::handle &in_l) -> bool {
        return in_r.get<time_point_wrap>() < in_l.get<time_point_wrap>();
      });
  p_i->user_handle.clear();
  for (auto &&l_u : p_i->list_sort_time) {
    auto l_user = l_u.get<assets_file>().user_attr();
    /// \brief 收集用户的配置
    p_i->user_handle[l_user].emplace_back(l_u);
  }

  if (!p_i->attendance_ptr) p_i->attendance_ptr = std::make_shared<business::attendance_rule>();

  if (p_i->list.empty()) {
    g_windows_manage().create_windows<show_message>("筛选后为空");
    return false;
  }

  /// \brief 这里设置一下时钟规则
  auto l_begin = p_i->list_sort_time.front().get<time_point_wrap>().current_month_start();
  auto l_end   = p_i->list_sort_time.back().get<time_point_wrap>().current_month_end();
  auto l_size  = p_i->user_handle.size();
  /// 显示一下进度条
  auto &l_p    = g_reg()->ctx().emplace<process_message>();
  l_p.set_name("开始计算数据");
  l_p.set_state(l_p.run);
  DOODLE_LOG_INFO("开始计算时间 {} -> {}", l_begin, l_end);
  p_i->user_size = l_size;
  for (const auto &item : p_i->user_handle) {
    p_i->attendance_ptr->async_get_work_clock(
        item.first, l_begin, l_end,
        [l_handle = item.first, l_size,
         this](const boost::system::error_code &in_code, const business::work_clock &in_clock) {
          auto &l_p = g_reg()->ctx().emplace<process_message>();
          l_p.message(fmt::format("完成用户 {} 时间获取", l_handle.get<user>().get_name()));
          l_p.progress_step({1, l_size});

          if (in_code) {
            l_p.set_state(l_p.fail);

            g_windows_manage().create_windows<show_message>(fmt::format("{}", in_code.what()));
            return;
          }
          if ((--p_i->user_size) == 0) {
            boost::asio::post(g_io_context(), [this]() { generate_table(); });
          }

          l_handle.get_or_emplace<business::work_clock>() = in_clock;
          DOODLE_LOG_INFO("用户 {} 时间规则 {}", l_handle.get<user>().get_name(), in_clock.debug_print());
        }
    );
  }

  return true;
}
void xlsx_export_widgets::filter_() {
  auto l_view  = g_reg()->view<database, assets_file, time_point_wrap>();

  auto l_begin = p_i->combox_month.time_data.current_month_start();
  auto l_end   = p_i->combox_month.time_data.current_month_end();
  DOODLE_LOG_INFO("开始日期 {} 结束日期 {}", l_begin, l_end);

  p_i->list =
      l_view |
      ranges::views::transform([](const entt::entity &in_tuple) -> entt::handle { return make_handle(in_tuple); }) |
      ranges::to_vector;

  p_i->list = p_i->list | ranges::views::filter([&](const entt::handle &in_handle) -> bool {
                auto &&l_t = in_handle.get<time_point_wrap>();
                return l_t <= l_end && l_t >= l_begin;
              }) |
              ranges::views::filter([&](const entt::handle &in_handle) -> bool {
                if (p_i->combox_user_id.data == "all")
                  return in_handle.get<assets_file>().user_attr().all_of<user>();
                else {
                  auto l_user = p_i->combox_user_id.current_user;
                  return in_handle.get<assets_file>().user_attr() == l_user;
                }
              }) |
              ranges::to_vector;
}
void xlsx_export_widgets::gen_user() {
  p_i->combox_user_id.user_list.clear();

  auto l_v = g_reg()->view<database, user>();
  for (auto &&[e, l_d, l_u] : l_v.each()) {
    if (l_u.get_name().empty()) continue;
    auto l_h = make_handle(e);
    p_i->combox_user_id.user_list.emplace(l_u.get_name(), make_handle(e));
  }
  p_i->combox_user_id.user_list.emplace("all", entt::handle{});
}

}  // namespace doodle::gui

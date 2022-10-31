//
// Created by TD on 2022/2/17.
//
#include "csv_export_widgets.h"

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

#include <doodle_app/gui/base/ref_base.h>
#include <doodle_app/gui/open_file_dialog.h>
#include <doodle_app/gui/show_message.h>
#include <doodle_app/lib_warp/imgui_warp.h>

#include <doodle_lib/attendance/attendance_dingding.h>
#include <doodle_lib/attendance/attendance_rule.h>

#include <boost/contract.hpp>
#include <boost/fusion/adapted/struct/adapt_struct.hpp>
#include <boost/fusion/include/adapt_struct.hpp>

#include <fmt/chrono.h>

namespace doodle::gui {
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

class csv_line_statistics_gui {
 public:
  csv_line_statistics_gui() = default;
  std::string user_name;
  std::string len_time;
};

}  // namespace doodle::gui
BOOST_FUSION_ADAPT_STRUCT(
    ::doodle::gui::csv_line_gui, organization_, user_, project_season_name_, episodes_, shot_, start_time_, end_time_,
    len_time_, time_info_, comment_info_, file_path_, name_attr_, cutoff_attr_
);

BOOST_FUSION_ADAPT_STRUCT(::doodle::gui::csv_line_statistics_gui, user_name, len_time);

namespace doodle::gui {

namespace csv_export_widgets_ns {
csv_line::csv_line(
    const entt::handle &in_handle, const entt::handle &in_up_time_handle, const entt::handle &in_user_handle,
    bool in_use_first_as_project_name, const std::string_view &in_season_fmt_str,
    const std::string_view &in_episodes_fmt_str, const std::string_view &in_shot_fmt_str
) {
  in_user_handle.any_of<user, dingding::user>() ? void()
                                                : throw_error(error_enum::component_missing_error, "缺失用户组件"s);
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

  using time_rational = boost::rational<std::uint64_t>;
  time_rational l_time_rational{chrono::floor<chrono::seconds>(k_time).count(), 60ull * 60ull * 8ull};

  organization_        = in_user_handle.all_of<dingding::user>() ? in_user_handle.get<dingding::user>().department_name
                                                                 : k_ass.organization_attr();
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
bool csv_line::operator<(const csv_line &in_l) const {
  return std::tie(project_season_name_, episodes_, shot_) <
         std::tie(in_l.project_season_name_, in_l.episodes_, in_l.shot_);
}
bool csv_line::operator==(const csv_line &in_l) const {
  return std::tie(project_season_name_, episodes_, shot_) ==
         std::tie(in_l.project_season_name_, in_l.episodes_, in_l.shot_);
}

void csv_table::computing_time() {
  ranges::for_each(line_list, [&](const csv_export_widgets_ns::csv_line &in_line) {
    time_statistics[in_line.user_] += in_line.len_time_;
  });

  DOODLE_LOG_INFO("计算时间总计 {}", fmt::join(time_statistics, " "));
  std::map<std::string, std::vector<std::size_t>> user_index;

  for (std::size_t l = 0; l < line_list.size(); ++l) {
    user_index[line_list[l].user_].emplace_back(l);
  }

  // 这里我们需要进行求整
  for (auto &&item : time_statistics) {
    using time_rational = boost::rational<std::uint64_t>;
    time_rational l_time_rational{item.second.count(), 60ull * 60ull * 8ull};

    auto l_round_value = std::round(boost::rational_cast<std::float_t>(l_time_rational));

    if (auto l_val = l_time_rational - time_rational{boost::numeric_cast<std::uint64_t>(l_round_value)}; l_val) {
      auto l_se = (l_val * 60ull * 60ull * 8ull).numerator();
      while (l_se) {
        for (auto i : user_index[item.first]) {
          line_list[i].len_time_++;
          if (--l_se) break;
        }
      }
    }
  }
}
void csv_table::sort_line() { line_list |= ranges::actions::sort; }

std::string csv_table::to_str() const {
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
                  line_list | ranges::view::transform([](const csv_line &in_line) -> std::string {
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
}  // namespace csv_export_widgets_ns

class csv_table_gui {
 public:
  gui_cache<csv_export_widgets::csv_table> gui_data{"预览"s};
  gui_cache_name_id show_details{"显示详细"};
  gui_cache_name_id show_statistics{"显示统计"};

  std::variant<std::vector<csv_line_gui>, std::vector<csv_line_statistics_gui>> list;

  void set_table_data(const std::vector<csv_export_widgets::csv_line> &in_csv_line) {
    gui_data().line_list = in_csv_line;
    gui_data().computing_time();
    gui_data().sort_line();
  }

  [[nodiscard]] constexpr std::int32_t size() const {
    return std::holds_alternative<std::vector<csv_line_gui>>(list) ? 13 : 2;
  }

  void render() {
    if (ImGui::Button(*show_details)) {
      list = gui_data().line_list |
             ranges::view::transform([](const csv_export_widgets_ns::csv_line &in_line) -> csv_line_gui {
               using time_rational = boost::rational<std::uint64_t>;
               time_rational l_time_rational{in_line.len_time_.count(), 60ull * 60ull * 8ull};

               return csv_line_gui{
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
             ranges::view::transform(
                 [](const std::pair<std::string, chrono::seconds> &in_line) -> csv_line_statistics_gui {
                   using time_rational = boost::rational<std::uint64_t>;
                   time_rational l_time_rational{in_line.second.count(), 60ull * 60ull * 8ull};
                   return csv_line_statistics_gui{
                       in_line.first, fmt::to_string(boost::rational_cast<std::double_t>(l_time_rational))};
                 }
             ) |
             ranges::to_vector;
    }

    dear::Table{*gui_data, size()} && [this]() { std::visit(*this, list); };
  }
  void operator()(const std::vector<csv_line_gui> &in_line) const {
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
    ranges::for_each(in_line, [](const csv_line_gui &in_gui) {
      ImGui::TableNextRow();

      boost::fusion::for_each(in_gui, [](const std::string &in_string) {
        ImGui::TableNextColumn();
        dear::Text(in_string);
      });
    });
  }

  void operator()(const std::vector<csv_line_statistics_gui> &in_vector) const {
    ImGui::TableSetupScrollFreeze(0, 1);  // 顶行可见
    ImGui::TableSetupColumn("用户");
    ImGui::TableSetupColumn("总时间");
    ImGui::TableHeadersRow();
    ranges::for_each(in_vector, [](const csv_line_statistics_gui &in_gui) {
      ImGui::TableNextRow();

      ImGui::TableNextColumn();
      dear::Text(in_gui.user_name);
      ImGui::TableNextColumn();
      dear::Text(in_gui.len_time);
    });
  }
};

class work_clock_method_gui {
 public:
  constexpr static std::string_view dingding{"钉钉软件"};
  constexpr static std::string_view rule_method{"规则生成"};

  gui_cache<std::string> data{"获取工作时间方法"s, std::string{dingding}};
  csv_export_widgets::work_clock_method method{csv_export_widgets::work_clock_method::form_dingding};
};

class csv_export_widgets::impl {
 public:
  class time_cache : public gui_cache<std::int32_t> {
   public:
    time_cache() : gui_cache<std::int32_t>("月份"s, 0){};
    time_point_wrap time_data{};
  };

  class user_list_cache : public gui_cache<std::string> {
   public:
    user_list_cache() : gui_cache<std::string>("过滤用户"s, "all"s){};
    std::map<std::string, entt::handle> user_list{};
    entt::handle current_user{};
  };

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
  gui_cache_name_id advanced_setting{"高级设置"};

  csv_table_gui csv_table_gui_{};
  work_clock_method_gui work_clock_method_gui_{};
  std::shared_ptr<business::detail::attendance_interface> attendance_ptr{};

  user_list_cache combox_user_id{};
  time_cache combox_month{};
  gui_cache_name_id filter{"过滤"};
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

  if (g_reg()->ctx().contains<std::vector<entt::handle>>()) p_i->list = g_reg()->ctx().at<std::vector<entt::handle>>();
  p_i->con.emplace_back(g_reg()->ctx().at<core_sig>().select_handles.connect([this](const std::vector<entt::handle> &in
                                                                             ) { p_i->list = in; }));
  p_i->export_path.path = FSys::temp_directory_path() / "tset.csv";
  p_i->export_path.data = p_i->export_path.path.generic_string();

  auto l_v              = g_reg()->view<database, user>();
  for (auto &&[e, l_d, l_u] : l_v.each()) {
    p_i->combox_user_id.user_list.emplace(l_u.get_name(), make_handle(e));
  }
  p_i->combox_user_id.user_list.emplace("all", entt::handle{});
  auto &&[l_y, l_m, l_d, l_h, l_mim, l_s] = p_i->combox_month.time_data.compose();
  p_i->combox_month()                     = l_m;
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

  dear::TreeNode{*p_i->advanced_setting} && [&]() {
    ImGui::Checkbox(*p_i->use_first_as_project_name.gui_name, &p_i->use_first_as_project_name.data);
    ImGui::InputText(*p_i->season_fmt_str.gui_name, &p_i->season_fmt_str.data);
    ImGui::InputText(*p_i->episodes_fmt_str.gui_name, &p_i->episodes_fmt_str.data);
    ImGui::InputText(*p_i->shot_fmt_str.gui_name, &p_i->shot_fmt_str.data);
    dear::Combo{*p_i->work_clock_method_gui_.data, p_i->work_clock_method_gui_.data().c_str()} && [&]() {
      for (const auto &item : {work_clock_method_gui::dingding, work_clock_method_gui::rule_method}) {
        if (ImGui::Selectable(item.data())) {
          p_i->work_clock_method_gui_.data = std::string{
              item == work_clock_method_gui::dingding ? work_clock_method_gui::dingding
                                                      : work_clock_method_gui::rule_method};
          p_i->work_clock_method_gui_.method = item == work_clock_method_gui::dingding
                                                   ? csv_export_widgets::work_clock_method::form_dingding
                                                   : csv_export_widgets::work_clock_method::form_rule;
        }
      }
    };
  };
  ImGui::PushItemWidth(100);
  if (ImGui::InputInt(*p_i->combox_month, &p_i->combox_month)) {
    auto &&[l_y, l_m, l_d, l_h, l_mim, l_s] = p_i->combox_month.time_data.compose();
    p_i->combox_month.time_data             = time_point_wrap{l_y, p_i->combox_month(), l_d, l_h, l_mim, l_s};
  }
  ImGui::SameLine();
  dear::Combo{*p_i->combox_user_id, p_i->combox_user_id().c_str()} && [this]() {
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
  }

  if (ImGui::Button(*p_i->gen_table)) {
    if (get_work_time()) generate_table();
  }
  ImGui::SameLine();
  if (ImGui::Button(*p_i->export_table)) {
    export_csv();
  }

  // 渲染表格
  p_i->csv_table_gui_.render();
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
  p_i->list |= ranges::actions::stable_sort([](const entt::handle &in_r, const entt::handle &in_l) -> bool {
    return in_r.get<assets_file>().user_attr().get<user>() < in_l.get<assets_file>().user_attr().get<user>();
  });
  p_i->csv_table_gui_.set_table_data(
      p_i->list | ranges::view::transform([this](const entt::handle &in_handle) -> csv_line {
        auto l_user = in_handle.get<assets_file>().user_attr();
        return csv_line{
            in_handle,
            get_user_up_time(l_user, in_handle),
            l_user,
            p_i->use_first_as_project_name(),
            p_i->season_fmt_str(),
            p_i->episodes_fmt_str(),
            p_i->shot_fmt_str()};
      }) |
      ranges::to_vector
  );
}
void csv_export_widgets::export_csv() {
  if (p_i->list.empty()) {
    DOODLE_LOG_INFO("选择为空, 不导出");
    return;
  }
  FSys::ofstream l_f{p_i->export_path()};
  l_f << p_i->csv_table_gui_.gui_data().to_str();
}
bool csv_export_widgets::get_work_time() {
  p_i->list_sort_time =
      ranges::copy(p_i->list) | ranges::actions::sort([](const entt::handle &in_r, const entt::handle &in_l) -> bool {
        return in_r.get<time_point_wrap>() < in_l.get<time_point_wrap>();
      });
  for (auto &&l_u : p_i->list_sort_time) {
    auto l_user = l_u.get<assets_file>().user_attr();
    /// \brief 收集用户的配置
    p_i->user_handle[l_user].emplace_back(l_u);
  }

  switch (p_i->work_clock_method_gui_.method) {
    case work_clock_method::form_dingding: {
      if (!p_i->attendance_ptr || !std::dynamic_pointer_cast<business::attendance_dingding>(p_i->attendance_ptr))
        p_i->attendance_ptr = std::make_shared<business::attendance_dingding>();

      if (!std::all_of(
              p_i->user_handle.begin(), p_i->user_handle.end(),
              [](const std::pair<entt::handle, std::vector<entt::handle>> &in_handle) {
                return in_handle.first.all_of<dingding::user>();
              }
          )) {
        auto l_msg   = std::make_shared<show_message>();
        auto l_users = p_i->user_handle |
                       ranges::view::filter([](const std::pair<entt::handle, std::vector<entt::handle>> &in_handle) {
                         return !in_handle.first.all_of<dingding::user>();
                       }) |
                       ranges::view::transform(
                           [](const std::pair<entt::handle, std::vector<entt::handle>> &in_handle) -> std::string {
                             return in_handle.first.get<user>().get_name();
                           }
                       ) |
                       ranges::to_vector;
        l_msg->set_message(fmt::format("缺失一下人员的电话号码:\n{}", fmt::join(l_users, "\n")));
        make_handle().emplace<gui_windows>() = l_msg;
        return false;
      }

      break;
    }
    case work_clock_method::form_rule: {
      if (!p_i->attendance_ptr || !std::dynamic_pointer_cast<business::attendance_rule>(p_i->attendance_ptr))
        p_i->attendance_ptr = std::make_shared<business::attendance_rule>();
      break;
    }
  }

  /// \brief 这里设置一下时钟规则
  auto l_begin = p_i->list_sort_time.front().get<time_point_wrap>().current_month_start();
  auto l_end   = p_i->list_sort_time.back().get<time_point_wrap>().current_month_end();
  for (const auto &item : p_i->user_handle) {
    if (!item.first.all_of<business::work_clock>()) {
      p_i->attendance_ptr->async_get_work_clock(
          item.first, l_begin, l_end,
          [l_handle = item.first](const boost::system::error_code &in_code, const business::work_clock &in_clock) {
            l_handle.get_or_emplace<business::work_clock>() = in_clock;
            DOODLE_LOG_INFO("用户 {} 时间规则 {}", l_handle.get<user>().get_name(), in_clock.debug_print());
          }
      );
    }
  }
  return true;
}
void csv_export_widgets::filter_() {
  auto l_view  = g_reg()->view<database, assets_file, time_point_wrap>();

  auto l_begin = p_i->combox_month.time_data.current_month_start();
  auto l_end   = p_i->combox_month.time_data.current_month_end();
  DOODLE_LOG_INFO("开始日期 {} 结束日期 {}", l_begin, l_end);

  p_i->list =
      l_view |
      ranges::view::transform([](const entt::entity &in_tuple) -> entt::handle { return make_handle(in_tuple); }) |
      ranges::to_vector;

  p_i->list = p_i->list | ranges::view::filter([&](const entt::handle &in_handle) -> bool {
                auto &&l_t = in_handle.get<time_point_wrap>();
                return l_t <= l_end && l_t >= l_begin;
              }) |
              ranges::view::filter([&](const entt::handle &in_handle) -> bool {
                if (p_i->combox_user_id.data == "all")
                  return true;
                else {
                  auto l_user = p_i->combox_user_id.current_user;
                  return in_handle.get<assets_file>().user_attr() == l_user;
                }
              }) |
              ranges::to_vector;

  auto l_v = g_reg()->view<database, user>();
  for (auto &&[e, l_d, l_u] : l_v.each()) {
    p_i->combox_user_id.user_list.emplace(l_u.get_name(), make_handle(e));
  }
  p_i->combox_user_id.user_list.emplace("all", entt::handle{});
}

}  // namespace doodle::gui

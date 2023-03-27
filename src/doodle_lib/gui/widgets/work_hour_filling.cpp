#include "work_hour_filling.h"

#include "doodle_core/core/core_help_impl.h"
#include "doodle_core/doodle_core_fwd.h"
#include "doodle_core/logger/logger.h"
#include "doodle_core/metadata/metadata.h"
#include "doodle_core/metadata/time_point_wrap.h"
#include "doodle_core/metadata/user.h"
#include "doodle_core/metadata/work_task.h"

#include "doodle_app/gui/base/base_window.h"
#include "doodle_app/gui/base/ref_base.h"
#include "doodle_app/gui/open_file_dialog.h"
#include "doodle_app/gui/show_message.h"
#include "doodle_app/lib_warp/imgui_warp.h"

#include <doodle_lib/distributed_computing/client.h>

#include <boost/asio/post.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/lambda2/lambda2.hpp>
#include <boost/numeric/conversion/cast.hpp>
#include <boost/operators.hpp>

#include "gui/widgets/derail/all_user_combox.h"
#include "gui/widgets/work_hour_filling.h"
#include "xlnt/xlnt.hpp"
#include <array>
#include <chrono>
#include <cstdint>
#include <date/date.h>
#include <date/tz.h>
#include <entt/entity/fwd.hpp>
#include <fmt/core.h>
#include <fmt/format.h>
#include <imgui.h>
#include <imgui_stdlib.h>
#include <map>
#include <memory>
#include <range/v3/action/sort.hpp>
#include <range/v3/action/unique.hpp>
#include <range/v3/algorithm/for_each.hpp>
#include <range/v3/algorithm/merge.hpp>
#include <range/v3/range/conversion.hpp>
#include <range/v3/view/cache1.hpp>
#include <range/v3/view/filter.hpp>
#include <range/v3/view/iota.hpp>
#include <range/v3/view/take.hpp>
#include <range/v3/view/transform.hpp>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>
#include <xlnt/cell/cell_reference.hpp>
#include <xlnt/utils/date.hpp>
#include <xlnt/utils/path.hpp>
#include <xlnt/workbook/workbook.hpp>

namespace doodle::gui {

namespace {

struct table_line : boost::totally_ordered<table_line> {
  inline const static std::map<std::string, std::string> tran{
      {"Monday"s, "星期一"s},  {"Tuesday", "星期二"s}, {"Wednesday", "星期三"s},
      {"Thursday", "星期四"s}, {"Friday", "星期五"s},  {"Saturday", "星期六"s},
      {"Sunday", "星期日"s},   {"AM", "上午"s},        {"PM", "下午"s}};
  table_line() = default;

  explicit table_line(const work_task_info& in_task)
      : data_handle(make_handle(in_task)),
        cache_time{in_task.time},
        time_day{fmt::format("{:%F}", cache_time)},
        week{tran.at(fmt::format("{:%A}", cache_time))},
        am_or_pm{tran.at(fmt::format("{:%p}", cache_time))},
        task{"##task"s, in_task.task_name},
        region{"##region"s, in_task.region},
        abstract{"##abstract"s, in_task.abstract} {}

  explicit table_line(const entt::handle& in_task)
      : data_handle(in_task),
        cache_time{in_task.get<work_task_info>().time},
        time_day{fmt::format("{:%F}", cache_time)},
        week{tran.at(fmt::format("{:%A}", cache_time))},
        am_or_pm{tran.at(fmt::format("{:%p}", cache_time))},
        task{"##task"s, in_task.get<work_task_info>().task_name},
        region{"##region"s, in_task.get<work_task_info>().region},
        abstract{"##abstract"s, in_task.get<work_task_info>().abstract} {}

  // explicit table_line(const time_point_wrap& in_task)
  //     : cache_time{chrono::round<chrono::hours>(in_task.get_local_time())},
  //       time_day{ fmt::format("{:%F}", cache_time)},
  //       week{ tran.at(fmt::format("星期 {:%w})", cache_time)},
  //       am_or_pm{ tran.at(fmt::format("{:%p}", cache_time))},
  //       task{"##task"s},
  //       region{"##region"s},
  //       abstract{"##abstract"s} {}

  explicit table_line(const chrono::time_point<chrono::system_clock, chrono::hours>& in_task)
      : data_handle{},
        cache_time{in_task},
        time_day{fmt::format("{:%F}", cache_time)},
        week{tran.at(fmt::format("{:%A}", cache_time))},
        am_or_pm{tran.at(fmt::format("{:%p}", cache_time))},
        task{"##task"s},
        region{"##region"s},
        abstract{"##abstract"s} {}

  explicit operator work_task_info() const { return work_task_info{user_handle, cache_time, task, region, abstract}; }

  bool operator==(const table_line& in) const { return cache_time == in.cache_time; }
  bool operator<(const table_line& in) const { return cache_time < in.cache_time; }

  chrono::time_point<chrono::system_clock, chrono::hours> cache_time{};

  entt::handle data_handle{};
  std::string time_day{};
  std::string week{};
  std::string am_or_pm{};
  gui_cache<std::string> task{"##task"s};
  gui_cache<std::string> region{"##region"s};
  gui_cache<std::string> abstract{"##abstract"s};
  entt::handle user_handle;
};
}  // namespace

class work_hour_filling::impl {
 public:
  std::string title{};
  bool open;

  /// @brief 过滤当前用户使用
  entt::handle current_user;
  /// @brief 使用时间过滤表id
  gui_cache<std::array<std::int32_t, 2>> time_month{"年.月", std::int32_t{1}, std::int32_t{1}};
  /// 表id
  gui_cache_name_id table{"工时信息"};
  /// 主要表的列表
  std::vector<table_line> table_list;

  /// 表头
  const std::array<std::string, 6> table_head{"日期"s, "星期"s, "时段"s, "项目"s, "地区"s, "工作内容摘要"s};

  /// 是否显示高级设置
  bool show_advanced_setting{};
  /// 高级设置id
  gui_cache_name_id advanced_setting{"高级设置"};

  /// 绘制用户组合框的类
  all_user_combox combox{true};

  /// 子窗口
  std::map<std::string, entt::handle> sub_windows{};
  /// 导出表格路径
  gui_cache<std::string> export_file_text{};
  /// 导出路径
  FSys::path export_path;
  /// 选择路径按钮
  gui_cache_name_id select_path_button{"选择路径"};
  /// 导出表按钮
  gui_cache_name_id export_button{"导出表格"};

  std::unique_ptr<distributed_computing::client> client_{};
};

work_hour_filling::work_hour_filling() : ptr(std::make_unique<impl>()) {
  ptr->title                 = std::string{name};
  ptr->show_advanced_setting = true;
  init();
}

void work_hour_filling::list_time(std::int32_t in_y, std::int32_t in_m) {
  using work_tub_t  = decltype(*g_reg()->view<work_task_info>().each().begin());

  auto l_time       = time_point_wrap{in_y, in_m, 1};
  auto l_begin_time = chrono::time_point_cast<chrono::hours>(l_time.current_month_start().get_sys_time());
  auto l_end_time   = chrono::time_point_cast<chrono::hours>(l_time.current_month_end().get_sys_time());
  if (!ptr->current_user) return;

  auto l_work_list =
      ptr->client_->get_user_work_task_info(ptr->current_user, ptr->current_user, std::pair{l_begin_time, l_end_time});
  /// 先生成
  ptr->table_list = l_work_list |
                    ranges::views::transform([&](const decltype(l_work_list)::value_type& in) -> table_line {
                      return table_line{in};
                    }) |
                    ranges::to_vector;
  /// 排序
  ptr->table_list |= ranges::actions::sort(boost::lambda2::_1 < boost::lambda2::_2);
}

void work_hour_filling::modify_item(std::size_t in_index) {
  auto&& l_i = ptr->table_list[in_index];

  DOODLE_LOG_INFO("编辑时间 {}", l_i.cache_time);
  l_i.data_handle.get_or_emplace<work_task_info>() = (work_task_info)l_i;
  /// todo:保存
}

void work_hour_filling::init() {
  ptr->client_ = std::make_unique<distributed_computing::client>(core_set::get_set().server_ip);
  try {
    ptr->current_user = ptr->client_->get_user(g_reg()->ctx().at<doodle::user::current_user>().uuid);
    //    ptr->current_user = g_reg()->ctx().at<doodle::user::current_user>().get_handle();
  } catch (const json_rpc::invalid_id_exception& in_err) {
    DOODLE_LOG_ERROR("无效的用户 {}", g_reg()->ctx().at<doodle::user::current_user>().uuid);
  }
  auto l_time       = time_point_wrap{}.compose();
  ptr->time_month() = {l_time.year, l_time.month};

  if (ptr->current_user) list_time(l_time.year, l_time.month);
}

void work_hour_filling::export_table(const FSys::path& in_path) {
  auto l_path = in_path;
  if (l_path.extension() != ".xlsx") {
    l_path.replace_extension(".xlsx");
  }

  const static std::vector<std::string> s_csv_header{"用户"s, "日期"s, "星期"s,        "时段"s,
                                                     "项目"s, "地区"s, "工作内容摘要"s};

  xlnt::workbook l_w{};

  auto l_table = l_w.active_sheet();
  l_w.title("工作内容"s);
  /// 添加头
  for (auto i = 0; i < s_csv_header.size(); ++i) {
    l_table.cell(xlnt::cell_reference(1 + i, 1)).value(s_csv_header[i]);
  }
  std::size_t l_index{2};
  auto l_task_ = g_reg()->view<database, work_task_info>();
  for (auto&& [l_e, l_d, l_w] : l_task_.each()) {
    /// 获取用户
    auto&& l_user = l_w.user_ref.user_attr().get<user>();
    l_table.cell(xlnt::cell_reference(1, l_index))
        .value(l_user.get_name().empty() ? fmt::format("匿名用户 {}", l_d.uuid()) : l_user.get_name());
    /// 其他几个
    chrono::year_month_day l_t{chrono::floor<chrono::days>(l_w.time)};
    l_table.cell(xlnt::cell_reference(2, l_index))
        .value(xlnt::date{
            (std::int32_t)l_t.year(), boost::numeric_cast<std::int32_t>((std::uint32_t)l_t.month()),
            boost::numeric_cast<std::int32_t>((std::uint32_t)l_t.day())});
    l_table.cell(xlnt::cell_reference(3, l_index)).value(table_line::tran.at(fmt::format("{:%A}", l_w.time)));
    l_table.cell(xlnt::cell_reference(4, l_index)).value(table_line::tran.at(fmt::format("{:%p}", l_w.time)));
    l_table.cell(xlnt::cell_reference(5, l_index)).value(l_w.task_name);
    l_table.cell(xlnt::cell_reference(6, l_index)).value(l_w.region);
    l_table.cell(xlnt::cell_reference(7, l_index)).value(l_w.abstract);
    ++l_index;
  }
  FSys::ofstream l_f{l_path, FSys::ofstream::binary};
  l_w.save(l_f);

  g_windows_manage().create_windows_arg(windows_init_arg{}
                                            .create<show_message>(fmt::format("完成导出表格 {}"s, l_path))
                                            .set_title("显示消息")
                                            .set_render_type<dear::Popup>());
}

const std::string& work_hour_filling::title() const { return ptr->title; }

bool work_hour_filling::render() {
  ImGui::Text("基本信息:");

  if (ImGui::InputInt2(*ptr->time_month, ptr->time_month().data())) {
    list_time(ptr->time_month()[0], ptr->time_month()[1]);
  };

  if (ptr->show_advanced_setting)
    dear::TreeNode{*ptr->advanced_setting} && [&]() {
      /// 打开新的窗口显示用户
      if (ptr->combox.render()) {
        auto l_user_h = ptr->combox.get_user();
        auto l_title  = l_user_h.get<user>().get_name().empty() ? fmt::format("匿名用户 {}", l_user_h)
                                                                : l_user_h.get<user>().get_name();
      }

      /// 导出表格功能
      if (ImGui::InputText(*ptr->export_file_text, &ptr->export_file_text)) {
        ptr->export_path = ptr->export_file_text();
      }
      ImGui::SameLine();
      if (ImGui::Button(*ptr->select_path_button)) {
        g_windows_manage().create_windows_arg(
            windows_init_arg{}
                .create<file_dialog>(file_dialog::dialog_args{}.set_use_dir().async_read([this](const FSys::path& in) {
                  ptr->export_path        = in / "tmp.xlsx";
                  ptr->export_file_text() = ptr->export_path.generic_string();
                }))
                .set_title("选择目录"s)
                .set_render_type<dear::Popup>()
                .set_flags(ImGuiWindowFlags_NoSavedSettings)

        );
      }
      if (!ptr->export_path.empty()) {
        ImGui::SameLine();
        if (ImGui::Button(*ptr->export_button)) {
          export_table(ptr->export_path);
        }
      }
    };

  ImGui::Text("工时信息");

  dear::Table{
      *ptr->table, boost::numeric_cast<std::int32_t>(ptr->table_head.size()),
      ImGuiTableFlags_::ImGuiTableFlags_RowBg} &&
      [&]() {
        ImGui::TableSetupScrollFreeze(0, 1);  // Make top row always visible
        ranges::for_each(ptr->table_head, [](const std::string& in) { ImGui::TableSetupColumn(in.c_str()); });
        ImGui::TableHeadersRow();

        for (auto i = 0ull; i < ptr->table_list.size(); ++i) {
          auto&& in = ptr->table_list[i];
          ImGui::TableNextRow();
          ImGui::TableNextColumn();
          dear::Text(in.time_day);
          ImGui::TableNextColumn();
          dear::Text(in.week);
          ImGui::TableNextColumn();
          dear::Text(in.am_or_pm);

          ImGui::TableNextColumn();
          if (dear::InputText(*in.task, &in.task)) modify_item(i);
          ImGui::TableNextColumn();
          if (dear::InputText(*in.region, &in.region)) modify_item(i);
          ImGui::TableNextColumn();
          if (dear::InputText(*in.abstract, &in.abstract)) modify_item(i);
        }
      };
  return ptr->open;
}

void work_hour_filling::show_advanced_setting(bool in_) { ptr->show_advanced_setting = in_; }

std::int32_t work_hour_filling::flags() {
  if (!ptr->show_advanced_setting)
    return ImGuiWindowFlags_NoSavedSettings;
  else
    return {};
}

void work_hour_filling::set_attr() {
  if (!ptr->show_advanced_setting) {
    ImGui::SetNextWindowSize({640, 360});
  }
}

work_hour_filling::~work_hour_filling() {
  boost::asio::post(g_io_context(), [l_sub = ptr->sub_windows]() {
    for (auto l_h : l_sub) {
      if (l_h.second) l_h.second.destroy();
    }
  });
}

}  // namespace doodle::gui
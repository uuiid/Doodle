//
// Created by TD on 2021/9/17.
//

#include "long_time_tasks_widget.h"

#include <doodle_core/core/app_base.h>
#include <doodle_core/core/doodle_lib.h>
#include <doodle_core/metadata/metadata_cpp.h>

#include <doodle_app/lib_warp/imgui_warp.h>

#include <boost/range.hpp>
#include <boost/range/algorithm_ext.hpp>

#include <fmt/chrono.h>

namespace doodle::gui {

boost::asio::awaitable<void> weite_tasks_info() {
  static chrono::sys_time_pos g_p{chrono::system_clock::now()};

  const static std::unordered_map<process_message::state, std::string_view> g_name{
      {process_message::state::fail, "失败"},
      {process_message::state::success, "成功"},
      {process_message::state::run, "运行中"},
      {process_message::state::wait, "等待中"},
      {process_message::state::pause, "暂停"}
  };
  boost::asio::as_tuple_t<boost::asio::use_awaitable_t<>>::as_default_on_t<boost::asio::system_timer> l_timer{
      co_await boost::asio::this_coro::executor
  };
  while ((co_await boost::asio::this_coro::cancellation_state).cancelled() == boost::asio::cancellation_type::none) {
    std::string l_info{
        R"(

<style>
    html,
    body {
        height: 100%;
    }

    body {
        margin: 0;
        background: linear-gradient(45deg, #49a09d, #5f2c82);
        font-family: sans-serif;
        font-weight: 100;
    }

    .container {
        position: absolute;
        top: 50%;
        left: 50%;
        transform: translate(-50%, -50%);
    }

    table {
        width: 800px;
        border-collapse: collapse;
        overflow: hidden;
        box-shadow: 0 0 20px rgba(0,0,0,0.1);
    }

    th,
    td {
        padding: 15px;
        background-color: rgba(255,255,255,0.2);
        color: #fff;
    }

    th {
        text-align: left;
    }

    thead {
        th {
            background-color: #55608f;
        }
    }

    tbody {
        tr {
            &:hover {
                background-color: rgba(255,255,255,0.3);
            }
        }
        td {
            position: relative;
            &:hover {
                &:before {
                    content: "";
                    position: absolute;
                    left: 0;
                    right: 0;
                    top: -9999px;
                    bottom: -9999px;
                    background-color: rgba(255,255,255,0.2);
                    z-index: -1;
                }
            }
        }
    }
</style>
<div class="container" role="region" tabindex="0">
  <table>
    <thead>
    <tr>
      <th>名称</th>
      <th>状态</th>
    </tr>
    </thead>
    <tbody>
)"
    };
    bool has_value{false};
    for (const auto&& [e, msg] : g_reg()->view<process_message>().each()) {
      l_info += fmt::format("<tr> <td>{}</td> <td>{}</td> </tr>", msg.get_name(), g_name.at(msg.get_state()));
      has_value |= true;
    }
    l_info += R"(</tbody>
  </table>
  <div style="margin-top:8px">Made with <a href="https://www.htmltables.io/" target="_blank">HTML Tables</a></div>
</div>)";
    if (has_value) {
      FSys::path l_path{
          FSys::get_cache_path("process_info") /
          fmt::format("{:%Y_%m_%d_%H_%M_%S}_{}.html", g_p, boost::this_process::get_id())
      };
      FSys::ofstream{l_path, std::ios::out | std::ios::trunc} << l_info;
    }

    l_timer.expires_after(1s);
    auto [l_ec] = co_await l_timer.async_wait();
    if (l_ec == boost::asio::error::operation_aborted) {
      break;
    }
  }
}

long_time_tasks_widget::long_time_tasks_widget() : p_current_select() {
  title_name_ = std::string{name};
  static std::once_flag l_flag;
  std::call_once(l_flag, []() {
    boost::asio::co_spawn(
        g_io_context(), weite_tasks_info(),
        boost::asio::bind_cancellation_slot(app_base::Get().on_cancel.slot(), boost::asio::detached)
    );
  });
}

bool long_time_tasks_widget::render() {
  static auto flags{
      ImGuiTableFlags_::ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_::ImGuiTableFlags_Resizable |
      ImGuiTableFlags_::ImGuiTableFlags_BordersOuter | ImGuiTableFlags_::ImGuiTableFlags_BordersV |
      ImGuiTableFlags_::ImGuiTableFlags_ContextMenuInBody
  };
  dear::Table{"long_time_tasks_widget", 6, flags, ImVec2{0, -350}} && [this]() {
    imgui::TableSetupColumn("名称");
    imgui::TableSetupColumn("进度");
    imgui::TableSetupColumn("消息");
    imgui::TableSetupColumn("状态");
    imgui::TableSetupColumn("时间");
    imgui::TableSetupColumn("动作");
    imgui::TableHeadersRow();

    for (const auto&& [e, msg] : g_reg()->view<process_message>().each()) {
      auto k_h = entt::handle{*g_reg(), e};
      imgui::TableNextRow();
      imgui::TableNextColumn();
      if (dear::Selectable(
              msg.get_name_id(), p_current_select == k_h,
              ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowItemOverlap
          )) {
        p_current_select = k_h;
      }

      imgui::TableNextColumn();
      dear::Text(fmt::format("{:04f}%", msg.get_progress_f()));

      imgui::TableNextColumn();
      auto&& l_message_back = msg.message_back();
      imgui::Text(l_message_back.data());

      imgui::TableNextColumn();
      //      dear::Text(std::string{magic_enum::enum_name(msg.get_state())});
      switch (msg.get_state()) {
        case process_message::state::wait:
          ImGui::Text("准备任务");
          break;
        case process_message::state::run:
          ImGui::Text("正在运行");
          break;
        case process_message::state::pause: {
          dear::WithStyleColor l_color{ImGuiCol_Text, ImVec4{1.0f, 1.0f, 0.0f, 1.0f}};
          ImGui::Text("暂停中...");
        } break;
        case process_message::state::fail: {
          dear::WithStyleColor l_color{ImGuiCol_Text, ImVec4{1.0f, 0.0f, 0.0f, 1.0f}};
          ImGui::Text("错误结束");
        } break;
        case process_message::state::success: {
          dear::WithStyleColor l_color{ImGuiCol_Text, ImVec4{0.0f, 1.0f, 0.0f, 1.0f}};
          ImGui::Text("成功完成");
        } break;
      }

      imgui::TableNextColumn();
      using namespace std::literals;
      dear::Text(
          msg.get_time() == chrono::sys_time_pos::duration{0} ? "..."s : fmt::format("{:%H:%M:%S}", msg.get_time())
      );

      ImGui::TableNextColumn();

      if (msg.is_connected()) {
        if (ImGui::SmallButton(fmt::format("关闭##{}", msg.get_name_id()).c_str())) msg.aborted();
      } else
        ImGui::Text("无");
    }
  };
  ImGui::Combo(
      "日志等级", reinterpret_cast<int*>(&index_),
      [](void* in_data, std::int32_t in_index, const char** out_text) -> bool {
        constexpr auto l_leve_names_tmp = magic_enum::enum_names<level::level_enum>();
        *out_text                       = l_leve_names_tmp[in_index].data();
        return true;
      },
      nullptr, static_cast<std::int32_t>(magic_enum::enum_count<level::level_enum>()) - 2
  );

  dear::Text("主要日志"s);
  if (dear::Child l_c{"main_log", ImVec2{0, 266}, true}; l_c) {
    if (p_current_select && p_current_select.any_of<process_message>()) {
      auto&& l_text = p_current_select.get<process_message>().level_log(index_);
      dear::TextWrapPos l_wrap{};
      imgui::TextUnformatted(l_text.data());
    }
  }

  return open;
}

const std::string& long_time_tasks_widget::title() const { return title_name_; }
} // namespace doodle::gui
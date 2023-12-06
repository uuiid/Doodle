//
// Created by TD on 2022/1/14.
//

#include "main_status_bar.h"

#include <doodle_core/core/status_info.h>
#include <doodle_core/thread_pool/process_message.h>

#include <boost/asio.hpp>

#include <lib_warp/imgui_warp.h>
#include <spdlog/sinks/base_sink.h>
#include <spdlog/spdlog.h>

/// \brief to https://github.com/ocornut/imgui/issues/1901
namespace ImGui {
bool BufferingBar(const char* label, float value, const ImVec2& size_arg, const ImU32& bg_col, const ImU32& fg_col) {
  ImGuiWindow* window = GetCurrentWindow();
  if (window->SkipItems) return false;

  ImGuiContext& g         = *GImGui;
  const ImGuiStyle& style = g.Style;
  const ImGuiID id        = window->GetID(label);

  ImVec2 pos              = window->DC.CursorPos;
  ImVec2 size             = size_arg;
  size.x -= style.FramePadding.x * 2;

  const ImRect bb(pos, ImVec2(pos.x + size.x, pos.y + size.y));
  ItemSize(bb, style.FramePadding.y);
  if (!ItemAdd(bb, id)) return false;

  // Render
  const float circleStart = size.x * 0.7f;
  const float circleEnd   = size.x;
  const float circleWidth = circleEnd - circleStart;

  window->DrawList->AddRectFilled(bb.Min, ImVec2(pos.x + circleStart, bb.Max.y), bg_col);
  window->DrawList->AddRectFilled(bb.Min, ImVec2(pos.x + circleStart * value, bb.Max.y), fg_col);

  const float t     = g.Time;
  const float r     = size.y / 2;
  const float speed = 1.5f;

  const float a     = speed * 0;
  const float b     = speed * 0.333f;
  const float c     = speed * 0.666f;

  const float o1    = (circleWidth + r) * (t + a - speed * (int)((t + a) / speed)) / speed;
  const float o2    = (circleWidth + r) * (t + b - speed * (int)((t + b) / speed)) / speed;
  const float o3    = (circleWidth + r) * (t + c - speed * (int)((t + c) / speed)) / speed;

  window->DrawList->AddCircleFilled(ImVec2(pos.x + circleEnd - o1, bb.Min.y + r), r, bg_col);
  window->DrawList->AddCircleFilled(ImVec2(pos.x + circleEnd - o2, bb.Min.y + r), r, bg_col);
  window->DrawList->AddCircleFilled(ImVec2(pos.x + circleEnd - o3, bb.Min.y + r), r, bg_col);
  return false;
}

bool Spinner(const char* label, float radius, int thickness, const ImU32& color) {
  ImGuiWindow* window = GetCurrentWindow();
  if (window->SkipItems) return false;

  ImGuiContext& g         = *GImGui;
  const ImGuiStyle& style = g.Style;
  const ImGuiID id        = window->GetID(label);

  ImVec2 pos              = window->DC.CursorPos;
  ImVec2 size((radius)*2, (radius + style.FramePadding.y) * 2);

  const ImRect bb(pos, ImVec2(pos.x + size.x, pos.y + size.y));
  ItemSize(bb, style.FramePadding.y);
  if (!ItemAdd(bb, id)) return false;

  // Render
  window->DrawList->PathClear();

  int num_segments    = 30;
  int start           = abs(ImSin(g.Time * 1.8f) * (num_segments - 5));

  const float a_min   = IM_PI * 2.0f * ((float)start) / (float)num_segments;
  const float a_max   = IM_PI * 2.0f * ((float)num_segments - 3) / (float)num_segments;

  const ImVec2 centre = ImVec2(pos.x + radius, pos.y + radius + style.FramePadding.y);

  for (int i = 0; i < num_segments; i++) {
    const float a = a_min + ((float)i / (float)num_segments) * (a_max - a_min);
    window->DrawList->PathLineTo(
        ImVec2(centre.x + ImCos(a + g.Time * 8) * radius, centre.y + ImSin(a + g.Time * 8) * radius)
    );
  }

  window->DrawList->PathStroke(color, false, thickness);
  return false;
}
}  // namespace ImGui

namespace doodle::gui {

namespace {

template <class Mutex>
class main_status_bar_sink : public ::spdlog::sinks::base_sink<Mutex> {
 public:
  boost::signals2::signal<void(std::string)> sig_{};

  void sink_it_(const spdlog::details::log_msg& msg) override {
    spdlog::memory_buf_t formatted;
    spdlog::sinks::base_sink<Mutex>::formatter_->format(msg, formatted);
    sig_(fmt::to_string(formatted));
  }
  void flush_() override { sig_(std::string{}); }
};

}  // namespace

using main_status_bar_sink_mt = main_status_bar_sink<std::mutex>;
class main_status_bar::impl {
 public:
  std::shared_ptr<boost::asio::high_resolution_timer> timer{};
  logger_ptr p_logger_;

  std::shared_ptr<main_status_bar_sink_mt> p_sink_{std::make_shared<main_status_bar_sink_mt>()};
  boost::signals2::scoped_connection p_connection_{};
  std::string show_str_{};
};

main_status_bar::main_status_bar() : p_i(std::make_unique<impl>()) {
  init();

  p_i->p_logger_     = g_windows_manage().logger();

  p_i->p_connection_ = p_i->p_sink_->sig_.connect([this](const std::string& in_string) {
    boost::asio::post(g_io_context(), [this, in_string]() { p_i->show_str_ = std::move(in_string); });
  });
  p_i->p_logger_->sinks().emplace_back(p_i->p_sink_);
}

void main_status_bar::init() { p_i->timer = std::make_shared<boost::asio::high_resolution_timer>(g_io_context()); }

bool main_status_bar::render() {
  dear::MenuBar{} && [&]() {
    /// \brief 渲染信息
    if (g_reg()->ctx().contains<status_info>()) {
      auto l_s = g_reg()->ctx().get<status_info>();
      if (l_s.need_save) {
        dear::Text("需要保存"s);
        ImGui::SameLine();
      }
      if (!l_s.message.empty()) {
        dear::Text(l_s.message);
        ImGui::SameLine();
      }
      dear::Text(fmt::format("{}/{}", l_s.select_size, l_s.show_size));
      ImGui::SameLine();
    }

    dear::Text(p_i->show_str_);
  };
  return true;
}


main_status_bar::~main_status_bar() = default;
}  // namespace doodle::gui

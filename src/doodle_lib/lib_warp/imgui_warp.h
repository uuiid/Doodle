//
// Created by TD on 2021/9/14.
//

#pragma once

#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>
#include <imgui_stdlib.h>
#include <doodle_lib/external/ImGuiFileDialog/ImGuiFileDialog.h>
#include <imguiwrap.dear.h>

namespace doodle {
namespace imgui {
using namespace ::ImGui;
using namespace IGFD;
}  // namespace imgui
namespace dear {
using namespace ::dear;
struct TreeNodeEx : public ScopeWrapper<TreeNodeEx> {
  bool use_dtor;

  template <class... Args>
  static bool find_flags(Args&&... in_args) {
    auto tub = std::make_tuple(std::forward<Args>(in_args)...);
    if constexpr (sizeof...(in_args) > 2) {
      return std::get<1>(tub) & ImGuiTreeNodeFlags_NoTreePushOnOpen;
    } else {
      return false;
    }
  };
  template <class... Args>
  TreeNodeEx(Args&&... in_args) noexcept
      : ScopeWrapper<TreeNodeEx>(
            ::ImGui::TreeNodeEx(std::forward<Args>(in_args)...)),
        use_dtor(find_flags(std::forward<Args>(in_args)...)) {}
  static void dtor() noexcept {};
  ~TreeNodeEx() {
    if (!ok_)
      return;

    if (!use_dtor)
      ImGui::TreePop();
  };
};

struct OpenPopup : public ScopeWrapper<OpenPopup> {
  template <class... Args>
  OpenPopup(Args&&... in_args) noexcept
      : ScopeWrapper<OpenPopup>(
            true) {
    ::ImGui::OpenPopup(std::forward<Args>(in_args)...);
  }

  static void dtor() noexcept {
    ImGui::EndPopup();
  };
};

struct TextWrapPos : public ScopeWrapper<TextWrapPos, true> {
  TextWrapPos(float wrap_pos_x)
      : ScopeWrapper<TextWrapPos, true>(true) {
    ImGui::PushTextWrapPos(wrap_pos_x);
  }
  static void dtor() noexcept {
    ImGui::PopTextWrapPos();
  };
};

struct HelpMarker : public ScopeWrapper<HelpMarker, true> {
  HelpMarker(const char* in_show_str, const char* in_args) noexcept
      : ScopeWrapper<HelpMarker, true>(false) {
    ImGui::TextDisabled(in_show_str);
    ItemTooltip{} && [&in_args]() {
      TextWrapPos{ImGui::GetFontSize() * 35.0f} && [&in_args]() {
        imgui::TextUnformatted(in_args);
      };
    };
  }
  HelpMarker(const char* in_args) noexcept
      : HelpMarker("(?)", in_args) {}
  HelpMarker(const std::string& in_args) noexcept
      : HelpMarker(in_args.c_str()){};

  HelpMarker(const std::string& in_show_str, const std::string& in_args) noexcept
      : HelpMarker(in_show_str.c_str(), in_args.c_str()){};
  static void dtor() noexcept {};
};

struct Disabled : public ScopeWrapper<HelpMarker> {
  Disabled(bool in_disabled = true) noexcept
      : ScopeWrapper<HelpMarker>(in_disabled) {
    imgui::BeginDisabled(in_disabled);
  };

  static void dtor() noexcept {
    imgui::EndDisabled();
  };
};
bool InputText(const char* label,
               std::filesystem::path* str,
               ImGuiInputTextFlags flags       = 0,
               ImGuiInputTextCallback callback = nullptr,
               void* user_data                 = nullptr);
}  // namespace dear
}  // namespace doodle

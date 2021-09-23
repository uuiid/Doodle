//
// Created by TD on 2021/9/14.
//

#pragma once

// clang-format off
#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>
#include <imgui_stdlib.h>
#include <DoodleLib/external/ImGuiFileDialog/ImGuiFileDialog.h>
#include <imguiwrap.dear.h>
// clang-format on

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

    if (use_dtor)
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

struct OpenFileDialog : public ScopeWrapper<OpenFileDialog> {
  template <class... Args>
  OpenFileDialog(const std::string& vKey, Args&&... in_args) noexcept
      : ScopeWrapper<OpenFileDialog>(
            ImGuiFileDialog::Instance()->Display(
                vKey.c_str(),
                std::forward<Args>(in_args)...)) {
  }

  static void dtor() noexcept {
    ImGuiFileDialog::Instance()->Close();
  };
};

}  // namespace dear
}  // namespace doodle

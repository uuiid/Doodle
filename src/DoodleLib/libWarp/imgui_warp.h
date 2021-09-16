//
// Created by TD on 2021/9/14.
//

#pragma once
#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>
#include <imgui_internal.h>
#include <imgui_stdlib.h>
#include <imguiwrap.dear.h>

namespace doodle {
namespace imgui = ::ImGui;
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
    if (use_dtor)
      ImGui::TreePop();
  };
};
}  // namespace dear
}  // namespace doodle

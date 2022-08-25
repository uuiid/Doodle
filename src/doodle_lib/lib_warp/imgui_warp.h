//
// github https://github.com/kfsone/imguiwrap
//

#pragma once

#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>
#include <imgui_stdlib.h>
namespace doodle {
namespace imgui {
using namespace ::ImGui;
}  // namespace imgui

namespace dear {
using namespace ::ImGui;
static const ImVec2 Zero(0.0f, 0.0f);

// scoped_effect is a helper that uses automatic object lifetime to control
// the invocation of a callable after potentially calling additional code,
// allowing for easy inline creation of scope guards.
//
// On its own, it does nothing but call the supplied function when it is
// destroyed;
template <typename Base, bool ForceDtor = false>
struct ScopeWrapper {
  using wrapped_type               = Base;
  using self_type                  = ScopeWrapper<Base>;

  static constexpr bool force_dtor = ForceDtor;

 protected:
  const bool ok_;

 public:
  // constructor takes a predicate that may be used to determine if
  // additional calls can be made, and a function/lambda/callable to
  // be invoked from the destructor.
  constexpr explicit ScopeWrapper(bool ok) noexcept : ok_{ok} {}

  // destructor always invokes the supplied destructor function.
  ~ScopeWrapper() noexcept {
    if constexpr (!force_dtor) {
      if (!ok_)
        return;
    }
    wrapped_type::dtor();
  }

  // operator&& will excute 'code' if the predicate supplied during
  // construction was true.
  template <typename PassthruFn>
  constexpr bool operator&&(PassthruFn passthru) const {
    if (ok_)
      passthru();
    return ok_;
  }

  constexpr operator bool() const noexcept { return ok_; }

 protected:
  ScopeWrapper(const ScopeWrapper&)            = delete;
  ScopeWrapper& operator=(const ScopeWrapper&) = delete;
};

struct IDScope : public ScopeWrapper<IDScope, true> {
  IDScope(const char* str_id)
      : ScopeWrapper(true) {
    ImGui::PushID(str_id);
  }

  IDScope(const char* str_id_begin, const char* str_id_end)
      : ScopeWrapper(true) {
    ImGui::PushID(str_id_begin, str_id_end);
  }
  IDScope(const void* ptr_id)
      : ScopeWrapper(true) {
    ImGui::PushID(ptr_id);
  }
  IDScope(int int_id)
      : ScopeWrapper(true) {
    ImGui::PushID(int_id);
  }

  static void dtor() noexcept { ImGui::PopID(); }
};

// Wrapper for ImGui::Begin ... End, which will always call End.
struct Begin : public ScopeWrapper<Begin, true> {
  // Invoke Begin and guarantee that 'End' will be called.
  explicit Begin(const char* title, bool* open = nullptr, ImGuiWindowFlags flags = 0) noexcept
      : ScopeWrapper(ImGui::Begin(title, open, flags)) {}
  static void dtor() noexcept { ImGui::End(); }
};

// Wrapper for ImGui::BeginChild ... EndChild, which will always call EndChild.
struct Child : public ScopeWrapper<Child, true> {
  explicit Child(const char* title, const ImVec2& size = Zero, bool border = false, ImGuiWindowFlags flags = 0) noexcept
      : ScopeWrapper(ImGui::BeginChild(title, size, border, flags)) {}
  explicit Child(ImGuiID id, const ImVec2& size = Zero, bool border = false, ImGuiWindowFlags flags = 0) noexcept
      : ScopeWrapper(ImGui::BeginChild(id, size, border, flags)) {}
  static void dtor() noexcept { ImGui::EndChild(); }
};

// Wrapper for ImGui::BeginChildFrame ... EndChildFrame, which will always call EndChildFrame.
struct ChildFrame : public ScopeWrapper<ChildFrame, true> {
  template <typename... Args>
  ChildFrame(Args&&... args) noexcept
      : ScopeWrapper(ImGui::BeginChildFrame(std::forward<Args>(args)...)) {}
  static void dtor() noexcept { ImGui::EndChildFrame(); }
};

// Wrapper for ImGui::BeginGroup ... EndGroup which will always call EndGroup.
struct Group : public ScopeWrapper<Group, true> {
  Group() noexcept : ScopeWrapper(true) { ImGui::BeginGroup(); }
  static void dtor() noexcept { ImGui::EndGroup(); }
};

// Wrapper for ImGui::Begin...EndCombo.
struct Combo : public ScopeWrapper<Combo> {
  Combo(const char* label, const char* preview, ImGuiComboFlags flags = 0) noexcept
      : ScopeWrapper(ImGui::BeginCombo(label, preview, flags)) {}
  static void dtor() noexcept { ImGui::EndCombo(); }
};

// Wrapper for ImGui::Begin...EndListBox.
struct ListBox : public ScopeWrapper<ListBox> {
  ListBox(const char* label, const ImVec2& size = Zero) noexcept
      : ScopeWrapper(ImGui::BeginListBox(label, size)) {}
  static void dtor() noexcept { ImGui::EndListBox(); }
};

// Wrapper for ImGui::Begin...EndMenuBar.
struct MenuBar : public ScopeWrapper<MenuBar> {
  MenuBar() noexcept : ScopeWrapper(ImGui::BeginMenuBar()) {}
  static void dtor() noexcept { ImGui::EndMenuBar(); }
};

// Wrapper for ImGui::Begin...EndMainMenuBar.
struct MainMenuBar : public ScopeWrapper<MainMenuBar> {
  MainMenuBar() noexcept : ScopeWrapper(ImGui::BeginMainMenuBar()) {}
  static void dtor() noexcept { ImGui::EndMainMenuBar(); }
};

// Wrapper for ImGui::BeginMenu...ImGui::EndMenu.
struct Menu : public ScopeWrapper<Menu> {
  Menu(const char* label, bool enabled = true) noexcept
      : ScopeWrapper(ImGui::BeginMenu(label, enabled)) {}
  static void dtor() noexcept { ImGui::EndMenu(); }
};

// Wrapper for ImGui::BeginTable...ImGui::EndTable.
// See also EditTableFlags.
struct Table : public ScopeWrapper<Table> {
  Table(const char* str_id, int column, ImGuiTableFlags flags = 0, const ImVec2& outer_size = Zero, float inner_width = 0.0f) noexcept
      : ScopeWrapper(ImGui::BeginTable(str_id, column, flags, outer_size, inner_width)) {}
  static void dtor() noexcept { ImGui::EndTable(); }
};

// Wrapper for ImGui::Begin...EndToolTip.
struct Tooltip : public ScopeWrapper<Tooltip> {
  Tooltip() noexcept : ScopeWrapper(true) { ImGui::BeginTooltip(); }
  static void dtor() noexcept { ImGui::EndTooltip(); }
};

// Wrapper around ImGui::CollapsingHeader to allow consistent code styling.
struct CollapsingHeader : public ScopeWrapper<CollapsingHeader> {
  CollapsingHeader(const char* label, ImGuiTreeNodeFlags flags = 0) noexcept
      : ScopeWrapper(ImGui::CollapsingHeader(label, flags)) {}
  inline static void dtor() noexcept {}
};

// Wrapper for ImGui::TreeNode...ImGui::TreePop.
// See also SeparatedTreeNode.
struct TreeNode : public ScopeWrapper<TreeNode> {
  template <typename... Args>
  TreeNode(Args&&... args) noexcept
      : ScopeWrapper(ImGui::TreeNode(std::forward<Args>(args)...)) {}
  static void dtor() noexcept { ImGui::TreePop(); }
};

// Wrapper around a TreeNode followed by a Separator (it's a fairly common sequence).
struct SeparatedTreeNode : public ScopeWrapper<SeparatedTreeNode> {
  template <typename... Args>
  SeparatedTreeNode(Args&&... args) noexcept
      : ScopeWrapper(ImGui::TreeNode(std::forward<Args>(args)...)) {}
  static void dtor() noexcept {
    ImGui::TreePop();
    ImGui::Separator();
  }
};

// Popup provides the stock wrapper around ImGui::BeginPopup...ImGui::EndPopup as well as two
// methods of instantiating a modal, for those who want modality to be a property fo Popup
// rather than a discrete type.
struct Popup : public ScopeWrapper<Popup> {
  // Non-modal Popup.
  Popup(const char* str_id, ImGuiWindowFlags flags = 0) noexcept
      : ScopeWrapper(ImGui::BeginPopup(str_id, flags)) {}

  // Modal popups.

  // imguiwrap provides 3 ways to construct a modal popup:
  // - Use the PopupModal class,
  // - Use Popup(modal{}, ...)
  // - Use the static method Popup::Modal(...)

  struct modal {
  };
  Popup(modal, const char* name, bool* p_open = nullptr, ImGuiWindowFlags flags = 0) noexcept
      : ScopeWrapper(ImGui::BeginPopupModal(name, p_open, flags)) {}

  static Popup
  Modal(const char* name, bool* p_open = nullptr, ImGuiWindowFlags flags = 0) noexcept {
    return Popup(modal{}, name, p_open, flags);
  }

  static void dtor() noexcept { ImGui::EndPopup(); }
};

// Wrapper around ImGui's BeginPopupModal ... EndPopup sequence.
struct PopupModal : public ScopeWrapper<PopupModal> {
  PopupModal(const char* name, bool* p_open = nullptr, ImGuiWindowFlags flags = 0) noexcept
      : ScopeWrapper(ImGui::BeginPopupModal(name, p_open, flags)) {}
  static void dtor() noexcept { ImGui::EndPopup(); }
};

struct PopupContextItem : public ScopeWrapper<PopupContextItem> {
  PopupContextItem(const char* str_id = NULL, ImGuiPopupFlags popup_flags = 1)
      : ScopeWrapper(ImGui::BeginPopupContextItem(str_id, popup_flags)) {}

  static void dtor() noexcept { ImGui::EndPopup(); }
};

// Wrapper for ImGui::BeginTabBar ... EndTabBar
struct TabBar : public ScopeWrapper<TabBar> {
  TabBar(const char* name, ImGuiTabBarFlags flags = 0) noexcept
      : ScopeWrapper(ImGui::BeginTabBar(name, flags)) {}
  static void dtor() noexcept { ImGui::EndTabBar(); }
};

// Wrapper for ImGui::BeginTabItem ... EndTabItem
struct TabItem : public ScopeWrapper<TabItem> {
  TabItem(const char* name, bool* open = nullptr, ImGuiTabItemFlags flags = 0) noexcept
      : ScopeWrapper(ImGui::BeginTabItem(name, open, flags)) {}
  static void dtor() noexcept { ImGui::EndTabItem(); }
};

// Wrapper around pushing a style var onto ImGui's stack and popping it back off.
/// TODO: Support nesting so we can do a single pop operation.
struct WithStyleVar : public ScopeWrapper<WithStyleVar> {
  WithStyleVar(ImGuiStyleVar idx, const ImVec2& val) noexcept : ScopeWrapper(true) {
    ImGui::PushStyleVar(idx, val);
  }
  WithStyleVar(ImGuiStyleVar idx, float val = 0.0f) noexcept : ScopeWrapper(true) {
    ImGui::PushStyleVar(idx, val);
  }
  static void dtor() noexcept { ImGui::PopStyleVar(); }
};

/// TODO: WithStyleColor

// Wrapper for BeginTooltip predicated on the previous item being hovered.
struct ItemTooltip : public ScopeWrapper<ItemTooltip> {
  ItemTooltip(ImGuiHoveredFlags flags = 0) noexcept
      : ScopeWrapper(ImGui::IsItemHovered(flags)) {
    if (ok_)
      ImGui::BeginTooltip();
  }
  static void dtor() noexcept { ImGui::EndTooltip(); }
};

//// Text helpers

// std::string helpers.
#ifndef DEAR_NO_STRING
static inline void Text(const std::string& str) noexcept {
  ImGui::TextUnformatted(str.c_str(), str.c_str() + str.length());
}
static inline void Text(const std::string_view& str) noexcept {
  ImGui::TextUnformatted(str.data(), str.data() + str.size());
}
inline void TextUnformatted(const std::string& str) noexcept {
  ImGui::TextUnformatted(str.c_str(), str.c_str() + str.length());
}
#endif

static inline bool
MenuItem(const char* text, bool selected = false, bool enabled = true) noexcept {
  return ImGui::MenuItem(text, nullptr, selected, enabled);
}
static inline bool MenuItem(const char* text, bool* selected, bool enabled = true) noexcept {
  return ImGui::MenuItem(text, nullptr, selected, enabled);
}
#ifndef DEAR_NO_STRING
static inline bool MenuItem(const std::string& str, const char* shortcut = nullptr, bool selected = false, bool enabled = true) noexcept {
  return ImGui::MenuItem(str.c_str(), shortcut, selected, enabled);
}
static inline bool
MenuItem(const std::string& text, bool* selected, bool enabled = true) noexcept {
  return ImGui::MenuItem(text.c_str(), nullptr, selected, enabled);
}
#endif

// static inline bool Selectable(const char *text) noexcept { return ImGui::Selectable(text); }
static inline bool
Selectable(const char* label, bool selected = false, ImGuiSelectableFlags flags = 0, const ImVec2& size = Zero) noexcept {
  return ImGui::Selectable(label, selected, flags, size);
}
static inline bool
Selectable(const char* label, bool* p_selected, ImGuiSelectableFlags flags = 0, const ImVec2& size = Zero) noexcept {
  return ImGui::Selectable(label, p_selected, flags, size);
}
#ifndef DEAR_NO_STRING
static inline bool
Selectable(const std::string& label, bool selected = false, ImGuiSelectableFlags flags = 0, const ImVec2& size = Zero) noexcept {
  return ImGui::Selectable(label.c_str(), selected, flags, size);
}
static inline bool
Selectable(const std::string& label, bool* p_selected, ImGuiSelectableFlags flags = 0, const ImVec2& size = Zero) noexcept {
  return ImGui::Selectable(label.c_str(), p_selected, flags, size);
}
#endif

/**
 * @brief  BeginViewportSideBar 签名接收 ImGuiViewport* viewport
 *       你可以改变这个：
 *                   ImGuiViewportP* viewport = (ImGuiViewportP*)(void*)ImGui::GetMainViewport();
 *       对此：
 *                   ImGuiViewport* viewport = ImGui::GetMainViewport();
 *       但是你可以只传递NULL，因为主视口是默认的。
 * @warning ImGui::BeginViewportSideBar 使用 ImGui::Begin 因此对 ImGui::End 的调用应该超出 {if 范围}
 *   ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_MenuBar;
 *   float height = ImGui::GetFrameHeight();
 */
struct ViewportSideBar : public ScopeWrapper<ViewportSideBar, true> {
  explicit ViewportSideBar(
      const std::string& in_name,
      ImGuiViewport* viewport, const ImGuiDir& dir, const float& size, const ImGuiWindowFlags& window_flags
  )
      : ScopeWrapper<ViewportSideBar, true>(
            BeginViewportSideBar(in_name.c_str(), viewport, dir, size, window_flags)
        ) {}
  static void dtor() noexcept { ImGui::End(); }
};

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
            ::ImGui::TreeNodeEx(std::forward<Args>(in_args)...)
        ),
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
            true
        ) {
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
 private:
  HelpMarker(const char* in_show_str, const char* in_args) noexcept
      : ScopeWrapper<HelpMarker, true>(false) {
    ::ImGui::SameLine();
    ::ImGui::TextDisabled(in_show_str);
    ItemTooltip{} && [&in_args]() {
      TextWrapPos{ImGui::GetFontSize() * 35.0f} && [&in_args]() {
        ImGui::TextUnformatted(in_args);
      };
    };
  }

 public:
  explicit HelpMarker(const std::string& in_args) noexcept
      : HelpMarker("(?)", in_args.c_str()){};

  explicit HelpMarker(const std::string& in_show_str, const std::string& in_args) noexcept
      : HelpMarker(in_show_str.c_str(), in_args.c_str()){};
  static void dtor() noexcept {};
};

struct Disabled : public ScopeWrapper<Disabled> {
  Disabled(bool in_disabled = true) noexcept
      : ScopeWrapper(in_disabled) {
    if (in_disabled) {
      ImGui::BeginDisabled(in_disabled);
    }
  };

  static void dtor() noexcept {
    ImGui::EndDisabled();
  };
};

struct DragDropSource : public ScopeWrapper<DragDropSource> {
 public:
  DragDropSource(ImGuiDragDropFlags flags = 0)
      : ScopeWrapper<DragDropSource>(ImGui::BeginDragDropSource(flags)) {}
  static void dtor() noexcept {
    ImGui::EndDragDropSource();
  }
};

struct DragDropTarget : public ScopeWrapper<DragDropTarget> {
 public:
  DragDropTarget()
      : ScopeWrapper<DragDropTarget>(ImGui::BeginDragDropTarget()) {}
  static void dtor() noexcept {
    ImGui::EndDragDropTarget();
  }
};

// bool InputText(const char* label,
//                FSys::path* str,
//                ImGuiInputTextFlags flags       = 0,
//                ImGuiInputTextCallback callback = nullptr,
//                void* user_data                 = nullptr);
}  // namespace dear
}  // namespace doodle

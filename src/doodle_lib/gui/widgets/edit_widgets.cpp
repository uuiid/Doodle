
//
// Created by TD on 2021/9/23.
//

#include "edit_widgets.h"

#include <doodle_lib/gui/action/command.h>
#include <doodle_lib/gui/action/command_files.h>
#include <doodle_lib/gui/action/command_meta.h>
#include <doodle_lib/gui/action/command_ue4.h>
#include <doodle_lib/gui/action/command_video.h>
#include <doodle_lib/gui/factory/attribute_factory_interface.h>
#include <doodle_lib/metadata/metadata.h>
#include <doodle_lib/core/core_sig.h>
#include <doodle_lib/gui/widgets/assets_file_widgets.h>

namespace doodle {

class edit_widgets::impl {
 public:
  boost::signals2::scoped_connection p_sc;

 public:
  /**
   * @brief 添加句柄
   *
   */
  std::int32_t p_add_size = 1;
  std::vector<entt::handle> add_handles;

 public:
  /**
   * @brief 修改句柄
   *
   */
  entt::handle p_h;
};

edit_widgets::edit_widgets()
    : p_i(std::make_unique<impl>()) {
}
edit_widgets::~edit_widgets() = default;

void edit_widgets::init() {
  g_reg()->set<edit_widgets &>(*this);
  p_i->p_sc = g_reg()
                  ->ctx<core_sig>()
                  .select_handle.connect(
                      [&](const entt::handle &in) {
                        p_i->p_h = in;
                      });
}
void edit_widgets::succeeded() {
  g_reg()->unset<edit_widgets &>();
  this->clear_handle();
}
void edit_widgets::failed() {
  g_reg()->unset<edit_widgets &>();
  this->clear_handle();
}
void edit_widgets::aborted() {
  g_reg()->unset<edit_widgets &>();
  this->clear_handle();
}
void edit_widgets::update(chrono::duration<chrono::system_clock::rep,
                                           chrono::system_clock::period>,
                          void *data) {
  dear::TreeNode{"添加"} && [this]() {
    this->add_handle();
  };
  dear::TreeNode{"编辑"} && [this]() {
    this->edit_handle();
  };
}

void edit_widgets::edit_handle() {
}

void edit_widgets::add_handle() {
  ImGui::InputInt("添加个数", &p_i->p_add_size);
  if (ImGui::Button("添加")) {
    for (std::int32_t i = 0; i < p_i->p_add_size; ++i) {
      p_i->add_handles.emplace_back(make_handle());
    }
    if (auto k_w = g_reg()->try_ctx<assets_file_widgets>(); k_w) {
      auto &k_list_h = k_w->get_handle_list();
      std::for_each(p_i->add_handles.begin(),
                    p_i->add_handles.end(),
                    std::back_inserter(k_list_h));
      auto k_rang = boost::unique(boost::sort(k_list_h));
      boost::erase(k_list_h, k_rang);
    }
  }
}

void edit_widgets::clear_handle() {
  if (auto k_w = g_reg()->try_ctx<assets_file_widgets>(); k_w) {
    std::for_each(p_i->add_handles.begin(),
                  p_i->add_handles.end(),
                  [](entt::handle &in) {
                    if (in.orphan()) {
                      in.destroy();
                    }
                  });
    auto &k_list_h = k_w->get_handle_list();
    std::vector<entt::handle> k_list{};
    auto k_rang = boost::sort(k_list_h) |
                  boost::adaptors::filtered(
                      [](const entt::handle &in) { return in.valid(); });
    boost::copy(k_rang, k_list);
    k_list_h = k_list;
  }
}
}  // namespace doodle

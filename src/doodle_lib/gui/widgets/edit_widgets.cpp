
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
#include <doodle_lib/metadata/assets_file.h>
#include <doodle_lib/gui/widgets/drag_widget.h>
#include <doodle_lib/long_task/drop_file_data.h>

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
  /**
   * @brief 添加多个
   *
   */
  ImGui::InputInt("添加个数", &p_i->p_add_size);
  ImGui::SameLine();
  if (ImGui::Button("添加")) {
    for (std::int32_t i = 0; i < p_i->p_add_size; ++i) {
      p_i->add_handles.emplace_back(make_handle());
    }
    this->notify_file_list();
  }

  /**
   * @brief 拖拽文件添加
   *
   */

  {
    dear::ListBox k_list{"文件列表"};
    k_list &&[this]() {
      for (auto &&i : p_i->add_handles) {
        if (i.all_of<assets_file>())
          dear::Text(i.get<assets_file>().p_name);
      }
    };
  }
  dear::DragDropTarget{} && [&]() {
    if (auto *l_pay = ImGui::AcceptDragDropPayload(doodle_config::drop_imgui_id.data()); l_pay) {
      auto k_list = reinterpret_cast<drop_file_data *>(l_pay->Data);
      boost::transform(k_list->files_,
                       std::back_inserter(p_i->add_handles),
                       [&](const FSys::path &in_path) { 
                         auto k_h  = make_handle();
                         auto& k_ass =  k_h.emplace<assets_file>(in_path.stem().generic_string());
                         k_ass.path = in_path;
                         return k_h; });
      DOODLE_LOG_INFO("检查到拖入文件:\n{}", fmt::join(k_list->files_, "\n"));
      this->notify_file_list();
    }
  };
}

void edit_widgets::clear_handle() {
  std::for_each(p_i->add_handles.begin(),
                p_i->add_handles.end(),
                [](entt::handle &in) {
                  if (in.orphan()) {
                    in.destroy();
                  }
                });
  boost::remove_erase_if(p_i->add_handles, [](const entt::handle &in) { return !in.valid(); });
  if (auto k_w = g_reg()->try_ctx<assets_file_widgets>(); k_w) {
    auto &k_list_h = k_w->get_handle_list();
    std::vector<entt::handle> k_list{};
    auto k_rang = boost::sort(k_list_h) |
                  boost::adaptors::filtered(
                      [](const entt::handle &in) { return in.valid(); });
    boost::copy(k_rang, std::back_inserter(k_list));
    k_list_h = k_list;
  }
}

void edit_widgets::notify_file_list() const {
  if (auto k_w = g_reg()->try_ctx<assets_file_widgets>(); k_w) {
    auto k_list_h = k_w->get_handle_list();
    k_w->get_handle_list().clear();

    boost::copy(p_i->add_handles, std::back_inserter(k_list_h));
    auto k_rang = boost::unique(boost::sort(k_list_h));

    boost::copy(boost::unique(boost::sort(k_list_h)),
                std::back_inserter(k_w->get_handle_list()));
  }
}
}  // namespace doodle

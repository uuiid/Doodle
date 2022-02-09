
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
#include <doodle_lib/metadata/metadata_cpp.h>

namespace doodle {

class assets_edit {
  class gui_chache {
   public:
    gui_chache(std::string in_name)
        : edit(in_name),
          button_name(fmt::format("删除##{}", fmt::ptr(this))),
          input_label(fmt::format("##编辑{}", fmt::ptr(this))),
          clear(false){};

    std::string edit;
    std::string button_name;
    std::string input_label;
    bool clear;
  };

 public:
  std::vector<gui_chache> p_cache;

  bool is_modify{false};

  void init(const entt::handle &in) {
    assets l_ass{"root"};
    if (in.all_of<assets>()) {
      l_ass = in.get<assets>();
    }
    p_cache.clear();
    for (auto &&i : l_ass.get_path_component()) {
      p_cache.emplace_back(i);
    }
  };
  void render(const entt::handle &in) {
    if (ImGui::Button("添加")) {
      p_cache.emplace_back("none");
      is_modify = true;
    }

    bool l_clear{false};

    dear::ListBox{"资产类别"} && [&]() {
      for (auto &&i : p_cache) {
        if (dear::InputText(i.input_label.c_str(), &i.edit))
          is_modify = true;
        ImGui::SameLine();
        if (dear::Button(i.button_name.c_str())) {
          i.clear = true;
          l_clear = true;
        }
      }
    };
    if (l_clear) {
      boost::remove_erase_if(p_cache, [](const gui_chache &in) { return in.clear; });
    }
  };

  void save(const entt::handle &in) const {
    if (is_modify) {
      std::vector<std::string> l_list;
      boost::transform(p_cache,
                       std::back_inserter(l_list),
                       [](const gui_chache &in)
                           -> std::string {
                         return in.edit;
                       });

      in.emplace_or_replace<assets>(FSys::path{fmt::to_string(fmt::join(l_list, "/"))});
    }
  }
};

class season_edit {
 public:
};
class episodes_edit {
 public:
};
class shot_edit {
 public:
};
class assets_file_edit {
 public:
};

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

  assets_edit p_ass_edit;
  season p_eason;
  episodes p_eps;
  shot p_shot;
  assets_file p_ass_file;
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
                        p_i->p_ass_edit.init(in);
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
  /// @brief 资产编辑
  dear::TreeNode{"资产编辑"} && [&]() {
    p_i->p_ass_edit.render(p_i->p_h);
  };
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
      bool l_clear{false};
      for (auto &&i : p_i->add_handles) {
        if (i.all_of<assets_file>()) {
          dear::Text(i.get<assets_file>().p_name);
          ImGui::SameLine();
          if (ImGui::Button(fmt::format("删除##{}", i.entity()).c_str())) {
            i.destroy();
            l_clear = true;
          }
        }
      }
      if (l_clear) {
        boost::remove_erase_if(p_i->add_handles,
                               [](const entt::handle &in) { return !in.valid(); });
        this->notify_file_list();
      }
    };
  }
  dear::DragDropTarget{} && [&]() {
    if (auto *l_pay = ImGui::AcceptDragDropPayload(doodle_config::drop_imgui_id.data()); l_pay) {
      auto k_list = reinterpret_cast<drop_file_data *>(l_pay->Data);
      boost::transform(k_list->files_,
                       std::back_inserter(p_i->add_handles),
                       [&](const FSys::path &in_path) {
                         auto k_h    = make_handle();
                         auto &k_ass = k_h.emplace<assets_file>(in_path.stem().generic_string());
                         k_ass.path  = in_path;
                         k_h.emplace<assets>("root");
                         return k_h;
                       });
      DOODLE_LOG_INFO("检查到拖入文件:\n{}", fmt::join(k_list->files_, "\n"));
      this->notify_file_list();
    }
  };

  if (ImGui::Button("保存")) {
    std::for_each(p_i->add_handles.begin(),
                  p_i->add_handles.end(),
                  [](entt::handle &in) {
                    if (!in.any_of<assets_file, assets, episodes, shot, season>()) {
                      in.destroy();
                    } else {
                      in.emplace_or_replace<database>();
                      in.patch<database>(database::save{});
                    }
                  });
    boost::remove_erase_if(p_i->add_handles,
                           [](const entt::handle &in) { return !in.valid(); });
    this->notify_file_list();
  }
}

void edit_widgets::clear_handle() {
  std::for_each(p_i->add_handles.begin(),
                p_i->add_handles.end(),
                [](entt::handle &in) {
                  if (in.orphan()) {
                    in.destroy();
                  }
                });
  boost::remove_erase_if(p_i->add_handles,
                         [](const entt::handle &in) { return !in.valid(); });
  this->notify_file_list();
}

void edit_widgets::notify_file_list() const {
  if (auto k_w = g_reg()->try_ctx<assets_file_widgets>(); k_w) {
    auto k_list_h = k_w->get_handle_list();
    k_w->get_handle_list().clear();

    boost::copy(p_i->add_handles, std::back_inserter(k_list_h));

    boost::copy(
        boost::unique(boost::sort(k_list_h)) |
            boost::adaptors::filtered([](const entt::handle &in) -> bool {
              return in.valid();
            }),
        std::back_inserter(k_w->get_handle_list()));
  }
}
}  // namespace doodle

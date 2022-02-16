//
// Created by TD on 2021/9/16.
//

#include "assets_file_widgets.h"

#include <doodle_lib/gui/action/command.h>
#include <doodle_lib/gui/action/command_files.h>
#include <doodle_lib/lib_warp/imgui_warp.h>
#include <doodle_lib/metadata/metadata_cpp.h>
#include <doodle_lib/core/image_loader.h>
#include <doodle_lib/metadata/image_icon.h>
#include <doodle_lib/long_task/process_pool.h>
#include <doodle_lib/gui/widgets/screenshot_widget.h>
#include <entt/entt.hpp>
#include <doodle_lib/long_task/drop_file_data.h>
#include <doodle_lib/core/core_sig.h>

#include <gui/gui_ref/ref_base.h>

namespace doodle {

class assets_file_widgets::impl {
 public:
  boost::signals2::scoped_connection p_sc;
  std::vector<entt::handle> handle_list;

  using cache_image  = gui::gui_cache<std::shared_ptr<void>>;
  using cache_name   = gui::gui_cache<std::string>;
  using cache_select = gui::gui_cache<bool>;
  class data {
   public:
    entt::handle handle_;
    cache_image image;
    cache_name name;
    cache_select select;

    explicit data(const entt::handle& in_h)
        : handle_(in_h),
          image("image"s, nullptr),
          name("null"s, "name"s),
          select(std::string{}, false) {
      if (handle_.all_of<assets_file>()) {
        /// @brief 渲染名称
        name = handle_.get<assets_file>().show_str();
      } else {
        /// @brief 否则渲染id
        if (handle_.all_of<database>())
          name = handle_.get<database>().get_id_str();
      }
    };

    void load_image() {
      image_loader k_load{};
      if (handle_.any_of<image_icon>()) {
        /// @brief 如果有图标就渲染
        auto&& k_icon = handle_.get<image_icon>();
        if (!k_icon.image)
          k_load.load(handle_);
        image = k_icon.image;
      } else {
        /// @brief 否则默认图标
        image = k_load.default_image();
      }
    };
  };
  std::vector<data> lists;
};

void assets_file_widgets::set_select(const entt::handle& in) {
}

assets_file_widgets::assets_file_widgets()
    : p_current_select(),
      p_i(std::make_unique<impl>()) {
}

void assets_file_widgets::init() {
  g_reg()->set<assets_file_widgets&>(*this);
  p_i->p_sc = g_reg()
                  ->ctx<core_sig>()
                  .filter_handle.connect(
                      [this](const std::vector<entt::handle>& in) {
                        p_i->handle_list = in;
                        p_i->lists.clear();
                        boost::transform(in, std::back_inserter(p_i->lists), [](const entt::handle& in) -> impl::data {
                          return impl::data{in};
                        });
                      });
}
void assets_file_widgets::succeeded() {
  g_reg()->unset<assets_file_widgets&>();
}
void assets_file_widgets::failed() {
  g_reg()->unset<assets_file_widgets&>();
}
void assets_file_widgets::aborted() {
  g_reg()->unset<assets_file_widgets&>();
}
void assets_file_widgets::update(chrono::duration<chrono::system_clock::rep, chrono::system_clock::period>, void* data) {
  if (p_i->lists.empty())
    return;

  const static auto l_size{5u};
  ImGui::Columns(l_size, "assets_file_widgets", false);

  image_loader k_load{};

  ImGuiListClipper clipper{};
  clipper.Begin((boost::numeric_cast<std::int32_t>(p_i->lists.size()) / l_size) + 1);
  while (clipper.Step()) {
    for (int l_i = clipper.DisplayStart; l_i < clipper.DisplayEnd; ++l_i) {
      for (int l_j = 0; l_j < l_size; ++l_j) {
        if ((l_i * l_size + l_j) < p_i->lists.size()) {
          auto&& i   = p_i->lists[l_i * l_size + l_j];
          i.load_image();
          auto l_pos = ImGui::GetCursorPos();
          ImGui::Selectable(i.select.name_id.c_str(), &i.select.data, ImGuiSelectableFlags_AllowItemOverlap, {66.f, 66.f});
          dear::PopupContextItem{} && [this, i]() {
            render_context_menu(i.handle_);
          };
          ImGui::SetCursorPos(l_pos);
          ImGui::Image(i.image.data.get(), {64.f, 64.f});
          dear::Text(i.name);
        }
        imgui::NextColumn();
      }
    }
  }
}

void assets_file_widgets::render_context_menu(const entt::handle& in_) {
  if (dear::MenuItem("打开") && in_.all_of<assets_file>()) {
    auto k_path = g_reg()->ctx<project>().get_path() / in_.get<assets_file>().path;
    FSys::open_explorer(FSys::is_directory(k_path) ? k_path : k_path.parent_path());
  }
  if (dear::MenuItem("截图")) {
    g_main_loop()
        .attach<screenshot_widget>(in_)
        .then<one_process_t>([=]() {
          in_.patch<database>(database::save);
        });
  }
}

assets_file_widgets::~assets_file_widgets() = default;

}  // namespace doodle

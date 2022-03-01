//
// Created by TD on 2021/9/16.
//

#include "assets_file_widgets.h"

#include <doodle_lib/gui/action/command.h>
#include <doodle_lib/lib_warp/imgui_warp.h>
#include <doodle_lib/metadata/metadata_cpp.h>
#include <doodle_lib/core/image_loader.h>
#include <doodle_lib/metadata/image_icon.h>
#include <doodle_lib/long_task/process_pool.h>
#include <doodle_lib/gui/widgets/screenshot_widget.h>
#include <doodle_lib/long_task/drop_file_data.h>
#include <doodle_lib/core/core_sig.h>
#include <doodle_lib/long_task/image_load_task.h>

#include <gui/gui_ref/ref_base.h>

namespace doodle {

class assets_file_widgets::impl {
 public:
  std::vector<boost::signals2::scoped_connection> p_sc;
  std::vector<entt::handle> handle_list;

  class image_data {
   public:
    cv::Size2f size2d_;
    cv::Size2f icon_size2d_;
    std::float_t max_;
  };

  using cache_image  = gui::gui_cache<std::shared_ptr<void>, image_data>;
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
          name = fmt::to_string(handle_.get<database>().get_id());
      }

      if (handle_.all_of<image_icon>() && !handle_.get<image_icon>().image) {
        g_main_loop()
            .attach<image_load_task>(handle_);
      }
    };

    void compute_size(float max_length) {
      if (image.max_ == max_length)
        return;
      else if (image.size2d_.empty()) {  /// 加载默认图标时大小为空， 直接指定大小
        image.icon_size2d_ = {max_length, max_length};
      } else {  /// 非默认图标直接计算大小
        if (image.size2d_.aspectRatio() >= 1) {
          image.icon_size2d_ = image.size2d_ * (max_length / image.size2d_.width);
        } else {
          image.icon_size2d_ = image.size2d_ * (max_length / image.size2d_.height);
        }
      }
      image.max_ = max_length;
    };

    void load_image(float max_length) {
      image_loader k_load{};
      if (handle_.any_of<image_icon>()) {
        /// @brief 如果有图标就获取
        auto&& k_icon = handle_.get<image_icon>();
        image         = k_icon.image;
        image.size2d_ = k_icon.size2d_;
        compute_size(max_length);
      } else {
        /// @brief 否则默认图标
        image = k_load.default_image();
        compute_size(max_length);
      }
    };
  };
  std::vector<data> lists;
  std::size_t select_index;

  // std::float_t windows_width{0};
  bool only_rand;
};

assets_file_widgets::assets_file_widgets()
    : p_current_select(),
      p_i(std::make_unique<impl>()) {
}

void assets_file_widgets::init() {
  g_reg()->set<assets_file_widgets&>(*this);
  auto& l_sig = g_reg()->ctx<core_sig>();
  p_i->p_sc.emplace_back(l_sig.filter_handle.connect(
      [this](const std::vector<entt::handle>& in) {
        p_i->handle_list = in;
        p_i->lists.clear();
        boost::transform(
            in, std::back_inserter(p_i->lists),
            [](const entt::handle& in) -> impl::data {
              return impl::data{in};
            });
      }));
  p_i->p_sc.emplace_back(l_sig.project_begin_open.connect(
      [&](const std::filesystem::path&) {
        p_i->handle_list.clear();
        p_i->lists.clear();
        p_i->select_index = 0;
      }));
  p_i->p_sc.emplace_back(
      l_sig.save_begin.connect(
          [&](const std::vector<entt::handle>&) {
            p_i->only_rand = true;
          }));
  p_i->p_sc.emplace_back(
      l_sig.save_end.connect(
          [&](const std::vector<entt::handle>&) {
            p_i->only_rand = false;
          }));
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
  /// 渲染数据
  dear::Disabled l_d{p_i->only_rand};

  if (p_i->lists.empty())
    return;

  const static auto l_size{5u};
  ImGui::Columns(l_size, "assets_file_widgets", false);

  auto k_length = (ImGui::GetCurrentWindow()->InnerClipRect.GetWidth() / l_size) - ImGui::GetStyle().ItemInnerSpacing.x * 3;

  ImGuiListClipper clipper{};
  clipper.Begin((boost::numeric_cast<std::int32_t>(p_i->lists.size() / l_size)) + 1);
  while (clipper.Step()) {
    for (int l_i = clipper.DisplayStart; l_i < clipper.DisplayEnd; ++l_i) {
      for (int l_j = 0; l_j < l_size; ++l_j) {
        if ((l_i * l_size + l_j) < p_i->lists.size()) {
          std::size_t l_index{l_i * l_size + l_j};
          auto&& i = p_i->lists[l_index];
          i.load_image(k_length);
          auto l_pos_image = ImGui::GetCursorPos();
          if (ImGui::Selectable(*i.select.gui_name,
                                &i.select.data,
                                ImGuiSelectableFlags_AllowDoubleClick,
                                {k_length, k_length}))
            set_select(l_index);
          dear::PopupContextItem{} && [this, i]() {
            render_context_menu(i.handle_);
          };
          dear::DragDropSource{} && [this, l_index]() {
            this->open_drag(l_index);
          };
          auto l_pos_select = ImGui::GetCursorPos();
          ImGui::SetCursorPos(l_pos_image);
          ImGui::Image(i.image.data.get(), {i.image.icon_size2d_.width - 2, i.image.icon_size2d_.height - 2});
          ImGui::SetCursorPos(l_pos_select);
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
  ImGui::Separator();
  if (dear::MenuItem("删除")) {
    in_.patch<database>(database::delete_);
    g_reg()->ctx<core_sig>().save_begin.connect([this, in_](const std::vector<entt::handle>&) {
      g_main_loop().attach<one_process_t>(
          [this, in_]() {
            p_i->lists = p_i->lists | ranges::views::remove_if([in_](const impl::data& in_data) {
                           return in_data.handle_ == in_;
                         }) |
                         ranges::to_vector;
          });
    });
  }
}
void assets_file_widgets::set_select(std::size_t in_size) {
  auto&& i   = p_i->lists[in_size];
  auto& k_io = imgui::GetIO();
  if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {  /// 双击鼠标时
    if (i.handle_.all_of<image_icon>())
      FSys::open_explorer(g_reg()->ctx<project>().make_path("image") / i.handle_.get<image_icon>().path);
  } else {  /// 单击鼠标时
    if (k_io.KeyCtrl) {
      i.select.data ^= 1;
      std::vector<entt::handle> l_h{};
      boost::copy(
          p_i->lists |
              boost::adaptors::filtered([](impl::data& in) {
                return in.select;
              }) |
              boost::adaptors::transformed([](impl::data& in) {
                return in.handle_;
              }),
          std::back_inserter(l_h));
      g_reg()->set<std::vector<entt::handle>>(l_h);
      g_reg()->ctx<core_sig>().select_handles(l_h);
    } else if (k_io.KeyShift) {
      std::vector<entt::handle> l_h{};
      boost::copy(
          p_i->lists |
              boost::adaptors::sliced(std::min(p_i->select_index, in_size),
                                      std::max(p_i->select_index, in_size) + 1) |
              boost::adaptors::transformed([](impl::data& in) {
                in.select = true;
                return in.handle_;
              }),
          std::back_inserter(l_h));
      g_reg()->set<std::vector<entt::handle>>(l_h);
      g_reg()->ctx<core_sig>().select_handles(l_h);
    } else {
      boost::for_each(p_i->lists,
                      [](impl::data& in) {
                        in.select = false;
                      });
      i.select = true;
    }
  }

  p_i->select_index = in_size;
  g_reg()->ctx<core_sig>().select_handle(i.handle_);
}
void assets_file_widgets::open_drag(std::size_t in_size) {
  auto l_item = p_i->lists[in_size];
  std::vector<entt::handle> l_lists{};
  l_lists = ranges::to_vector(
      this->p_i->lists |
      ranges::views::filter([](const impl::data& in) -> bool {
        return in.select;
      }) |
      ranges::views::transform(
          [](const impl::data& in) -> entt::handle {
            return in.handle_;
          }));
  l_lists.emplace_back(l_item.handle_);
  g_reg()->set<std::vector<entt::handle>>(l_lists);
  auto* l_h = g_reg()->try_ctx<std::vector<entt::handle>>();
  ImGui::SetDragDropPayload(
      doodle_config::drop_handle_list.data(), l_h, sizeof(*l_h));
  ImGui::Text("拖拽实体");
}

assets_file_widgets::~assets_file_widgets() = default;

}  // namespace doodle

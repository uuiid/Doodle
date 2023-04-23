//
// Created by TD on 2021/9/16.
//

#include "assets_file_widgets.h"

#include <doodle_core/core/core_sig.h>
#include <doodle_core/core/init_register.h>
#include <doodle_core/core/status_info.h>
#include <doodle_core/metadata/image_icon.h>
#include <doodle_core/metadata/metadata_cpp.h>

#include "doodle_app/gui/base/base_window.h"
#include <doodle_app/gui/base/ref_base.h>
#include <doodle_app/lib_warp/imgui_warp.h>

#include <doodle_lib/core/image_loader.h>
#include <doodle_lib/gui/widgets/screenshot_widget.h>
#include <doodle_lib/long_task/image_load_task.h>

#include "create_entry.h"
#include "entt/entity/fwd.hpp"
#include "imgui.h"
#include <memory>
#include <vector>

namespace doodle::gui {

class assets_file_widgets::impl {
 public:
  class base_data;
  class image_data;
  impl()              = default;
  virtual ~impl()     = default;
  using cache_image   = gui::gui_cache<std::shared_ptr<void>, image_data>;
  using cache_name    = gui::gui_cache<std::string>;
  using cache_select  = gui::gui_cache<bool>;
  using base_data_ptr = std::shared_ptr<base_data>;

  std::vector<boost::signals2::scoped_connection> p_sc;
  std::vector<entt::handle> handle_list;
  bool open{true};

  class image_data {
   public:
    cv::Size2f size2d_;
    cv::Size2f icon_size2d_;
    std::float_t max_{};
  };
  class base_data {
   public:
    explicit base_data(const entt::handle& in_h) : handle_(in_h), select(std::string{}, false) {}
    explicit base_data(const entt::handle& in_h, const std::string& in_string)
        : handle_(in_h), select(in_string, false) {}
    virtual ~base_data() = default;
    entt::handle handle_;
    cache_select select;
  };

  class icon_data : public base_data, public std::enable_shared_from_this<icon_data> {
   public:
    cache_image image;
    cache_name name;

    explicit icon_data(const entt::handle& in_h) : base_data(in_h), image("image"s, nullptr), name("null"s, "name"s) {
      this->init(in_h);
    };

    void init(const entt::handle& in_h) {
      if (handle_.any_of<assets_file, episodes, shot>()) {
        /// @brief 渲染名称
        if (auto [l_ass, l_ep, l_shot] = handle_.try_get<assets_file, episodes, shot>(); l_ass || l_ep || l_shot) {
          if (l_ass) {
            name.data = l_ass->name_attr();
            if (l_ep || l_shot) name.data.push_back('\n');
          }

          if (l_ep) {
            name.data += fmt::to_string(*l_ep);
          }
          if (l_shot) {
            name.data.push_back('_');
            name.data += fmt::to_string(*l_shot);
          }
        }
      } else {
        /// @brief 否则渲染id
        if (handle_.all_of<database>()) name = fmt::to_string(handle_.get<database>().get_id());
      }
    }

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
        if (!k_icon.image && !handle_.any_of<image_icon::image_load_tag>()) {
          handle_.emplace_or_replace<image_icon::image_load_tag>();
          g_reg()->ctx().at<image_load_task>().async_read(
              handle_,
              [handle_ = handle_, self = shared_from_this(), max_length]() {
                if (!self) return;
                auto&& k_icon       = handle_.get<image_icon>();
                self->image         = k_icon.image;
                self->image.size2d_ = k_icon.size2d_;
                self->compute_size(max_length);
              }
          );
          return;
        }
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

  class info_data : public base_data {
   public:
    explicit info_data(const entt::handle& in_h) : base_data(in_h, fmt::to_string(in_h.get<database>().get_id())) {
      init(in_h);
    }
    template <typename T>
    void to_s(std::string& in_p, const entt::handle& in_h) {
      if (in_h.all_of<T>())
        in_p = fmt::to_string(in_h.get<T>());
      else
        in_p = {};
    }

    void init(const entt::handle& in_h) {
      to_s<assets>(ass_p, in_h);
      to_s<episodes>(eps_p, in_h);
      to_s<shot>(shot_p, in_h);
      to_s<assets_file>(name_p, in_h);
      to_s<time_point_wrap>(time_p, in_h);
      if (in_h.any_of<assets_file>()) {
        auto&& l_ass = in_h.get<assets_file>();
        user_p       = l_ass.user_attr().get<user>().get_name();
        file_path_p  = in_h.get<assets_file>().get_path_normal().generic_string();
      }
    }

    std::string ass_p;
    std::string eps_p;
    std::string shot_p;
    std::string name_p;
    std::string file_path_p;
    std::string time_p;
    std::string user_p;
  };
  std::vector<base_data_ptr> lists;
  std::size_t select_index{};

  // std::float_t windows_width{0};

  bool render_icon{true};

  std::function<void()> render_list;
  //  entt::observer observer_h{*g_reg(), entt::collector.update<database>()};
  std::string title_name_;
};

assets_file_widgets::assets_file_widgets() : p_i(std::make_unique<impl>()) {
  p_i->title_name_ = std::string{name};
  g_reg()->ctx().emplace<image_load_task>();
  this->switch_rander();
  init();
}

void assets_file_widgets::switch_rander() {
  if (p_i->render_icon) {
    p_i->render_list = [this]() { this->render_by_icon(); };
  } else {
    p_i->render_list = [this]() { this->render_by_info(); };
  }
}

void assets_file_widgets::init() {
  g_reg()->ctx().emplace<assets_file_widgets&>(*this);
  auto& l_sig = g_reg()->ctx().at<core_sig>();
  p_i->p_sc.emplace_back(l_sig.filter_handle.connect([this](const std::vector<entt::handle>& in) {
    p_i->handle_list = in;
    generate_lists(p_i->handle_list);
  }));
  p_i->p_sc.emplace_back(l_sig.project_begin_open.connect([&](const FSys::path&) {
    p_i->handle_list.clear();
    p_i->lists.clear();
    p_i->select_index = 0;
  }));

}

bool assets_file_widgets::render() {
  /// 渲染数据
  const float footer_height_to_reserve = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
  auto* l_win_main                     = ImGui::GetCurrentWindow();
  if (auto l_drag = dear::DragDropTargetCustom{l_win_main->ContentRegionRect, l_win_main->ID}) {
    if (const auto* l_data = ImGui::AcceptDragDropPayload(doodle::doodle_config::drop_imgui_id.data());
        l_data && l_data->IsDelivery()) {
      auto* l_list = static_cast<std::vector<FSys::path>*>(l_data->Data);
      g_windows_manage().create_windows_arg(
          windows_init_arg{}
              .create<create_entry>(create_entry::init_args{}.set_paths(*l_list).set_create_call(
                  [this](const std::vector<entt::handle>& in_handle) {
                    p_i->handle_list = in_handle;
                    generate_lists(p_i->handle_list);
                  }
              ))
              .set_render_type<dear::Popup>()
      );
    }
  }

  {
    dear::Child l_win{"ScrollingRegion", ImVec2(0, -footer_height_to_reserve), false};
    l_win&& [&]() {
      if (p_i->lists.empty()) return;
      p_i->render_list();
    };
  }

  if (ImGui::Button(ICON_FA_ATOM)) {
    p_i->render_icon = !p_i->render_icon;
    generate_lists(p_i->handle_list);
    switch_rander();
  }
  g_reg()->ctx().at<status_info>().show_size = p_i->lists.size();

  return p_i->open;
}

void assets_file_widgets::render_context_menu(const entt::handle& in_) {
  if (dear::MenuItem("打开") && in_.all_of<assets_file>()) {
    auto k_path = g_reg()->ctx().at<project>().get_path() / in_.get<assets_file>().path_attr();
    FSys::open_explorer(FSys::is_directory(k_path) ? k_path : k_path.parent_path());
  }
  if (dear::MenuItem("截图")) {
    g_windows_manage().create_windows_arg(
        windows_init_arg{}.create<screenshot_widget>(in_).set_render_type<dear::Popup>()
    );
  }
  ImGui::Separator();
  if (dear::MenuItem("删除")) {
    std::vector<entt::handle> l_list =
        p_i->lists | ranges::views::indirect |
        ranges::views::filter([](const impl::base_data& in_data) { return in_data.select && in_data.handle_; }) |
        ranges::views::transform([](const impl::base_data& in_data) -> entt::handle { return in_data.handle_; }) |
        ranges::to_vector | ranges::actions::push_back(in_) | ranges::actions::unique | ranges::to_vector;
    ranges::for_each(l_list, [](const entt::handle& in_handle) { database::delete_(in_handle); });

    g_reg()->ctx().at<core_sig>().save_begin.connect([this, in_, l_list]() {
      boost::asio::post(g_io_context(), [this, in_, l_list]() {
        p_i->lists = p_i->lists | ranges::views::remove_if([l_list](const impl::base_data_ptr& in_data) {
                       return ranges::contains(l_list, in_data->handle_);
                     }) |
                     ranges::to_vector;
      });
    });
  }
}
void assets_file_widgets::set_select(std::size_t in_size) {
  auto&& i   = *p_i->lists[in_size];
  auto& k_io = imgui::GetIO();
  std::vector<entt::handle> l_handle_list{};
  if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {  /// 双击鼠标时
    if (i.handle_.all_of<image_icon>())
      FSys::open_explorer(g_reg()->ctx().at<project>().make_path("image") / i.handle_.get<image_icon>().path);
  } else {  /// 单击鼠标时
    if (k_io.KeyCtrl) {
      i.select.data = !i.select.data;
      l_handle_list = p_i->lists | ranges::views::indirect |
                      ranges::views::filter([](impl::base_data& in) { return in.select; }) |
                      ranges::views::transform([](impl::base_data& in) { return in.handle_; }) | ranges::to_vector;
    } else if (k_io.KeyShift) {
      l_handle_list =
          p_i->lists |
          ranges::views::slice(std::min(p_i->select_index, in_size), std::max(p_i->select_index, in_size) + 1) |
          ranges::views::indirect | ranges::views::transform([](impl::base_data& in) {
            in.select = true;
            return in.handle_;
          }) |
          ranges::to_vector;
    } else {
      ranges::for_each(p_i->lists | ranges::views::indirect, [](impl::base_data& in) { in.select = false; });
      i.select = true;
      l_handle_list.push_back(i.handle_);
    }
  }

  p_i->select_index = in_size;
  if (!l_handle_list.empty()) {
    g_reg()->ctx().erase<std::vector<entt::handle>>();
    g_reg()->ctx().emplace<std::vector<entt::handle>>(l_handle_list);
    auto& l_sig = g_reg()->ctx().at<core_sig>();
    l_sig.select_handles(l_handle_list);
    l_sig.select_handle(i.handle_);
    g_reg()->ctx().at<status_info>().select_size = l_handle_list.size();
  }
}
void assets_file_widgets::open_drag(std::size_t in_size) {
  auto&& l_item = *p_i->lists[in_size];
  std::vector<entt::handle> l_lists{};
  l_lists = ranges::to_vector(
      this->p_i->lists | ranges::views::indirect |
      ranges::views::filter([](const impl::base_data& in) -> bool { return in.select; }) |
      ranges::views::transform([](const impl::base_data& in) -> entt::handle { return in.handle_; })
  );
  if (ranges::find(l_lists, l_item.handle_) == l_lists.end()) l_lists.emplace_back(l_item.handle_);
  g_reg()->ctx().erase<std::vector<entt::handle>>();
  g_reg()->ctx().emplace<std::vector<entt::handle>>(l_lists);
  if (g_reg()->ctx().contains<std::vector<entt::handle>>()) {
    ImGui::SetDragDropPayload(
        doodle_config::drop_handle_list.data(), &(g_reg()->ctx().at<std::vector<entt::handle>>()),
        sizeof(g_reg()->ctx().at<std::vector<entt::handle>>())
    );
    ImGui::Text("拖拽实体");
  }
}

void assets_file_widgets::render_by_icon() {
  const static auto l_size{5u};
  ImGui::Columns(l_size, "assets_file_widgets", false);
  auto k_length =
      (ImGui::GetCurrentWindow()->InnerClipRect.GetWidth() / l_size) - ImGui::GetStyle().ItemInnerSpacing.x * 3;
  ImGuiListClipper clipper{};
  clipper.Begin((boost::numeric_cast<std::int32_t>(p_i->lists.size() / l_size)) + 1);
  while (clipper.Step()) {
    for (int l_i = clipper.DisplayStart; l_i < clipper.DisplayEnd; ++l_i) {
      for (int l_j = 0; l_j < l_size; ++l_j) {
        if ((l_i * l_size + l_j) < p_i->lists.size()) {
          std::size_t l_index{l_i * l_size + l_j};
          auto&& i = *std::dynamic_pointer_cast<impl::icon_data>(p_i->lists[l_index]);
          i.load_image(k_length);
          auto l_pos_image = ImGui::GetCursorPos();

          ImGui::PushStyleColor(ImGuiCol_Header, (ImVec4)ImColor::HSV(7.0f, 0.6f, 0.8f));

          if (ImGui::Selectable(
                  *i.select.gui_name, i.select.data, ImGuiSelectableFlags_AllowDoubleClick, {k_length, k_length}
              )) {
            set_select(l_index);
          }
          dear::PopupContextItem{} && [this, &i]() { render_context_menu(i.handle_); };
          ImGui::PopStyleColor();

          dear::DragDropSource{} && [this, l_index]() { this->open_drag(l_index); };
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
void assets_file_widgets::render_by_info() {
  const static auto l_size{8u};

  dear::Table{
      "list",
      l_size,
      ImGuiTableFlags_::ImGuiTableFlags_ScrollY | ImGuiTableFlags_::ImGuiTableFlags_ScrollX |
          ImGuiTableFlags_::ImGuiTableFlags_RowBg | ImGuiTableFlags_::ImGuiTableFlags_BordersOuter |
          ImGuiTableFlags_::ImGuiTableFlags_BordersV | ImGuiTableFlags_::ImGuiTableFlags_Resizable |
          ImGuiTableFlags_::ImGuiTableFlags_Reorderable | ImGuiTableFlags_::ImGuiTableFlags_Hideable,
  } && [&]() {
    ImGui::TableSetupScrollFreeze(0, 1);  // Make top row always visible
    ImGui::TableSetupColumn("id", ImGuiTableColumnFlags_NoHide);
    ImGui::TableSetupColumn("分类", ImGuiTableColumnFlags_None);
    ImGui::TableSetupColumn("集数", ImGuiTableColumnFlags_None);
    ImGui::TableSetupColumn("镜头", ImGuiTableColumnFlags_None);
    ImGui::TableSetupColumn("路径", ImGuiTableColumnFlags_None);
    ImGui::TableSetupColumn("名称", ImGuiTableColumnFlags_None);
    ImGui::TableSetupColumn("时间", ImGuiTableColumnFlags_None);
    ImGui::TableSetupColumn("制作人", ImGuiTableColumnFlags_None);
    ImGui::TableHeadersRow();

    ImGuiListClipper clipper{};
    clipper.Begin(boost::numeric_cast<std::int32_t>(p_i->lists.size()));
    while (clipper.Step()) {
      for (auto l_i = clipper.DisplayStart; l_i < clipper.DisplayEnd; ++l_i) {
        std::size_t l_index{boost::numeric_cast<std::size_t>(l_i)};
        auto&& i = *std::dynamic_pointer_cast<impl::info_data>(p_i->lists[l_index]);
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        if (ImGui::Selectable(
                *i.select.gui_name, i.select.data,
                ImGuiSelectableFlags_::ImGuiSelectableFlags_SpanAllColumns |
                    ImGuiSelectableFlags_::ImGuiSelectableFlags_AllowDoubleClick
            )) {
          set_select(l_index);
        }
        dear::PopupContextItem{} && [this, &i]() { render_context_menu(i.handle_); };
        dear::DragDropSource{} && [this, l_index]() { this->open_drag(l_index); };

        ImGui::TableNextColumn();
        dear::Text(i.ass_p);
        ImGui::TableNextColumn();
        dear::Text(i.eps_p);
        ImGui::TableNextColumn();
        dear::Text(i.shot_p);
        ImGui::TableNextColumn();
        dear::Text(i.file_path_p);
        ImGui::TableNextColumn();
        dear::Text(i.name_p);
        ImGui::TableNextColumn();
        dear::Text(i.time_p);
        ImGui::TableNextColumn();
        dear::Text(i.user_p);
      }
    }
  };
}
void assets_file_widgets::generate_lists(const std::vector<entt::handle>& in_list) {
  if (p_i->render_icon)
    p_i->lists = in_list | ranges::views::transform([](const entt::handle& in) -> impl::base_data_ptr {
                   return std::make_shared<impl::icon_data>(in);
                 }) |
                 ranges::to_vector;
  else
    p_i->lists = in_list | ranges::views::transform([](const entt::handle& in) -> impl::base_data_ptr {
                   return std::make_shared<impl::info_data>(in);
                 }) |
                 ranges::to_vector;
}

const std::string& assets_file_widgets::title() const { return p_i->title_name_; }

assets_file_widgets::~assets_file_widgets() { /*p_i->observer_h.disconnect();*/
}

}  // namespace doodle::gui

//
// Created by TD on 2022/9/21.
//

#include "create_video.h"

#include <doodle_core/core/core_set.h>
#include <doodle_core/core/core_sig.h>
#include <doodle_core/core/doodle_lib.h>
#include <doodle_core/metadata/assets_file.h>
#include <doodle_core/metadata/episodes.h>
#include <doodle_core/metadata/season.h>
#include <doodle_core/metadata/shot.h>

#include <doodle_app/gui/base/ref_base.h>
#include <doodle_app/gui/open_file_dialog.h>

#include <doodle_lib/exe_warp/maya_exe.h>
#include <doodle_lib/long_task/connect_video.h>
#include <doodle_lib/long_task/image_to_move.h>

#include <utility>

namespace doodle::gui {
class create_video::image_arg : public gui::gui_cache<std::string> {
public:
  using base_type = gui::gui_cache<std::string>;

  explicit image_arg(
    const entt::handle& in_handle, std::vector<FSys::path> in_image_attr, const std::string& in_show_str
  )
    : base_type(in_show_str), out_handle(in_handle), image_attr(std::move(in_image_attr)) {
  };

  entt::handle out_handle;
  std::vector<FSys::path> image_attr;
};

class video_gui_cache : boost::less_than_comparable<video_gui_cache> {
public:
  explicit video_gui_cache(std::string in_name, std::string in_data, FSys::path in_path)
    : gui_name_(std::move(in_name)), data_(std::move(in_data)), path_(std::move(in_path)) {
  };
  gui_cache_name_id gui_name_;
  std::string data_;
  FSys::path path_;

  bool operator<(const video_gui_cache& in_r) const { return path_ < in_r.path_; }
};

class create_video::impl {
public:
  using image_cache = create_video::image_arg;
  using video_cache = video_gui_cache;

  std::vector<image_cache> image_to_video_list;
  std::vector<video_cache> video_list;
  std::string title_name_;
  bool open{true};
};

create_video::create_video() : p_i(std::make_unique<impl>()) { p_i->title_name_ = std::string{name}; }

bool create_video::render() {
  ImGui::Text("拖拽文件夹, 或者图片列表, 由系统自动判断输出路径");
  constexpr auto l_sort_file_stem = [](const video_gui_cache& in_r, const video_gui_cache& in_l) -> bool {
    episodes l_eps_r{};
    l_eps_r.analysis(in_r.path_.stem());
    shot l_shot_r{};
    l_shot_r.analysis(in_r.path_.stem());

    episodes l_eps_l{};
    l_eps_l.analysis(in_l.path_.stem());
    shot l_shot_l{};
    l_shot_l.analysis(in_l.path_.stem());

    return std::tie(l_eps_r, l_shot_r) < std::tie(l_eps_l, l_shot_l);
  };
  if (auto l_l = dear::ListBox{"图片路径(拖拽导入)"}; l_l) {
    for (const auto& i : p_i->image_to_video_list) {
      dear::Selectable(*i.gui_name);
    }
  }
  if (auto l_drag          = dear::DragDropTarget{}) {
    if (const auto* l_data = ImGui::AcceptDragDropPayload(doodle_config::drop_imgui_id.data());
      l_data && l_data->IsDelivery()) {
      auto* l_list = static_cast<std::vector<FSys::path>*>(l_data->Data);

      for (auto&& l_file : *l_list) {
        if (FSys::is_directory(l_file)) {
          std::vector<FSys::path> l_list_files;
          for (auto&& l_p : FSys::recursive_directory_iterator(l_file)) {
            if (FSys::is_regular_file(l_p) && (l_p.path().extension() == ".png" || l_p.path().extension() == ".jpg" ||
                                               l_p.path().extension() == ".jpeg"))
              l_list_files.push_back(l_p);
          }

          auto&& l_out = p_i->image_to_video_list.emplace_back(
            create_image_to_move_handle(l_file), l_list_files, l_file.generic_string()
          );
        }
      }
      if (ranges::all_of(*l_list, [](const FSys::path& in_path) -> bool { return FSys::is_regular_file(in_path); })) {
        auto&& l_out = p_i->image_to_video_list.emplace_back(
          create_image_to_move_handle(l_list->front().parent_path()), *l_list, l_list->front().generic_string()
        );
      }
    }
  }
  if (ImGui::Button("清除")) {
    boost::asio::post(g_io_context(), [this]() { p_i->image_to_video_list.clear(); });
  }
  ImGui::SameLine();
  if (ImGui::Button("创建视频")) {
    boost::asio::post(g_io_context(), [this]() { p_i->image_to_video_list.clear(); });

    ranges::for_each(p_i->image_to_video_list, [=, this](const impl::image_cache& in_cache) {
      in_cache.out_handle.emplace<process_message>(
        in_cache.out_handle.get<image_to_move::element_type::out_file_path>().path.filename().generic_string()
      );
      g_reg()->ctx().get<image_to_move>()->async_create_move(
        in_cache.out_handle, in_cache.image_attr,

        boost::asio::bind_executor(
          g_io_context(),
          [this, l_h = in_cache.out_handle, l_sort_file_stem](
        const FSys::path& in_path, const boost::system::error_code& in_error_code
      ) {
            /// \brief 在这里我们将合成的视频添加到下一个工具栏中
            if (in_error_code) {
              l_h.get<process_message>().logger()->error("创建视频失败: {}", in_error_code.message());
              return;
            }
            if (ranges::count_if(p_i->video_list, [&](const video_gui_cache& in_gui_cache) {
              return in_gui_cache.path_ == in_path;
            }) == 0) {
              p_i->video_list.emplace_back(in_path.filename().generic_string(), in_path.generic_string(), in_path);
              p_i->video_list |= ranges::actions::sort(l_sort_file_stem);
            }
          }
        )
      );
    });
  }

  dear::ListBox{"视屏路径(拖拽导入)"} && [this]() {
    for (const auto& i : p_i->video_list) {
      dear::Selectable(*i.gui_name_);
    }
  };
  if (auto l_drag          = dear::DragDropTarget{}) {
    if (const auto* l_data = ImGui::AcceptDragDropPayload(doodle_config::drop_imgui_id.data());
      l_data && l_data->IsDelivery()) {
      auto* l_list = static_cast<std::vector<FSys::path>*>(l_data->Data);
      p_i->video_list.clear();
      for (auto&& l_file : *l_list) {
        if (FSys::is_regular_file(l_file) && l_file.extension() == ".mp4") {
          p_i->video_list.emplace_back(l_file.filename().generic_string(), l_file.generic_string(), l_file);
          p_i->video_list |= ranges::actions::sort(l_sort_file_stem);
        }
      }
    }
  }

  if (ImGui::Button("清除视频")) {
    boost::asio::post(g_io_context(), [this]() { p_i->video_list.clear(); });
  }
  ImGui::SameLine();
  if (ImGui::Button("连接视频")) {
    auto l_list = p_i->video_list |
                  ranges::views::transform([](impl::video_cache& in_cache) -> FSys::path { return in_cache.path_; }) |
                  ranges::to_vector;
    if (!l_list.empty()) {
      boost::asio::post(g_io_context(), [this]() { p_i->video_list.clear(); });
      auto l_out_video_h = entt::handle{*g_reg(), g_reg()->create()};
      l_out_video_h.remove<episodes>();
      ranges::any_of(p_i->video_list, [l_out_video_h](impl::video_cache& in_cache) -> bool {
        return episodes::analysis_static(l_out_video_h, in_cache.path_);
      });
      l_out_video_h.emplace_or_replace<connect_video::element_type::out_file_path>(l_list.front().parent_path());
      if (!l_out_video_h.all_of<process_message>())
        l_out_video_h.emplace<process_message>(
          l_out_video_h.get<connect_video::element_type::out_file_path>().path.filename().generic_string()
        );

      if (!g_ctx().contains<connect_video>())
        g_ctx().emplace<connect_video>(std::make_shared<detail::connect_video_t>());

      g_ctx().get<connect_video>()->async_connect_video(
        l_out_video_h, l_list,
        boost::asio::bind_executor(
          g_io_context(),
          [this, l_out_video_h](const FSys::path& in_path, const boost::system::error_code& in_error_code) {
            if (in_error_code) l_out_video_h.get<process_message>().logger()->error(
              "连接视频失败: {}", in_error_code.message());
            default_logger_raw()->info("完成视频合成 {}", in_path);
          }
        )
      );
    }
  }

  return p_i->open;
}

entt::handle create_video::create_image_to_move_handle(const FSys::path& in_path) {
  boost::ignore_unused(this);
  auto l_h = entt::handle{*g_reg(), g_reg()->create()};
  season::analysis_static(l_h, in_path);
  episodes::analysis_static(l_h, in_path);
  shot::analysis_static(l_h, in_path);

  l_h.emplace_or_replace<image_to_move::element_type::out_file_path>(in_path.parent_path());

  return l_h;
}

const std::string& create_video::title() const { return p_i->title_name_; }
create_video::~create_video() = default;
} // namespace doodle::gui
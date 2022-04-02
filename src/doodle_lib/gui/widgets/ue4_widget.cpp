//
// Created by TD on 2022/4/1.
//

#include "ue4_widget.h"
#include <doodle_lib/gui/gui_ref/ref_base.h>
#include <doodle_lib/lib_warp/imgui_warp.h>
#include <doodle_lib/long_task/process_pool.h>
#include <doodle_lib/gui/open_file_dialog.h>
#include <doodle_lib/metadata/assets_file.h>
#include <doodle_lib/metadata/episodes.h>
#include <doodle_lib/metadata/shot.h>
#include <doodle_lib/app/app.h>
#include <doodle_lib/core/program_options.h>
#include <doodle_lib/core/filesystem_extend.h>


#include <boost/contract.hpp>

namespace doodle {
class ue4_widget::impl {
  class file_data {
   public:
    file_data() = default;
    entt::handle handle_;
  };

 public:
  impl()             = default;
  using ue4_file_gui = gui::gui_cache<bool, file_data>;

  gui::gui_cache<std::string, gui::gui_cache_path> ue4_prj{"ue4项目"s, ""s};
  gui::gui_cache<std::vector<ue4_file_gui>> import_list_files{"##list"s, std::vector<ue4_file_gui>{}};

  gui::gui_cache<bool> import_cam{"导入cam"s, true};
  gui::gui_cache<bool> import_abc{"导入abc"s, true};
  gui::gui_cache<bool> import_fbx{"导入fbx"s, true};
  gui::gui_cache<bool> quit_{"生成并退出"s, true};
  gui::gui_cache_name_id import_{"导入"s};
  gui::gui_cache_name_id open_file_dig{"选择"s};

 public:
  FSys::path ue4_content_dir{};
};

ue4_widget::ue4_widget()
    : p_i(std::make_unique<impl>()) {
}
ue4_widget::~ue4_widget() = default;

void ue4_widget::init() {
  g_reg()->set<ue4_widget &>(*this);
}

void ue4_widget::succeeded() {
  g_reg()->unset<ue4_widget>();
}

void ue4_widget::failed() {
  g_reg()->unset<ue4_widget>();
}

void ue4_widget::aborted() {
  g_reg()->unset<ue4_widget>();
}

void ue4_widget::update(
    const std::chrono::duration<
        std::chrono::system_clock::rep,
        std::chrono::system_clock::period> &,
    void *data) {
  if (ImGui::InputText(*p_i->ue4_prj.gui_name, &p_i->ue4_prj.data)) {
    p_i->ue4_prj.path    = p_i->ue4_prj.data;
    p_i->ue4_content_dir = p_i->ue4_prj.path.parent_path() / doodle_config::ue4_content;
  }
  ImGui::SameLine();
  if (ImGui::Button(*p_i->open_file_dig)) {
    auto l_p = std::make_shared<FSys::path>();
    g_main_loop().attach<file_dialog>(
                     file_dialog::dialog_args{l_p}
                         .set_use_dir()
                         .add_filter(".uproject"s))
        .then<one_process_t>([this, l_p]() {
          this->p_i->ue4_prj.data = l_p->generic_string();
          this->p_i->ue4_prj.path = *l_p;
        });
  }
  /// 列出文件
  dear::ListBox{*p_i->import_list_files.gui_name} && [this]() {
    for (auto &&i : p_i->import_list_files.data) {
      ImGui::Selectable(*i.gui_name, &i.data);
    }
  };
  dear::DragDropTarget{} && [this]() {
    if (auto *l_pay = ImGui::AcceptDragDropPayload(doodle_config::drop_handle_list.data())) {
      auto l_list = reinterpret_cast<std::vector<entt::handle> *>(l_pay->Data);
      if (l_list) {
        this->accept_handle(*l_list);
      }
    }
  };

  /// 各种选项
  ImGui::Checkbox(*p_i->import_cam.gui_name, &p_i->import_cam.data);
  ImGui::Checkbox(*p_i->import_fbx.gui_name, &p_i->import_fbx.data);
  ImGui::Checkbox(*p_i->import_abc.gui_name, &p_i->import_abc.data);
  ImGui::Checkbox(*p_i->quit_.gui_name, &p_i->quit_.data);

  /// 开始导入
  if (ImGui::Button(*p_i->import_)) {
    this->import_ue4_prj();
  }
}
void ue4_widget::import_ue4_prj() {
  auto l_list =
      p_i->import_list_files.data |
      ranges::views::transform([](const auto &in_item) -> entt::handle {
        return in_item.handle_;
      }) |
      ranges::views::filter([](const entt::handle &in_handle) -> bool {
        return in_handle && in_handle.any_of<assets_file>();
      }) |
      ranges::to_vector;
  ranges::for_each(l_list,
                   [this](const entt::handle &in_handle) {
                     this->plan_file_path(in_handle.get<assets_file>().path, in_handle);
                   });
}
void ue4_widget::accept_handle(const std::vector<entt::handle> &in_list) {
  p_i->import_list_files.data =
      in_list |
      ranges::views::filter([](const entt::handle &in_handle) -> bool {
        return in_handle && in_handle.any_of<assets_file>();
      }) |
      ranges::views::transform([](const entt::handle &in_handle) -> impl::ue4_file_gui {
        auto str = in_handle.get<assets_file>().p_name;
        impl::ue4_file_gui l_gui{str, false};
        l_gui.handle_ = in_handle;
        return l_gui;
      }) |
      ranges::to_vector;
}
void ue4_widget::plan_file_path(const FSys::path &in_path,
                                const entt::handle &in_handle) {
  using namespace ue4_widget_n;
  ue4_import_group l_group{};
  l_group.groups =
      ranges::make_subrange(
          FSys::directory_iterator{in_path},
          FSys::directory_iterator{}) |
      ranges::views::filter(
          [](const FSys::directory_entry &in_entry) -> bool {
            return in_entry.path().extension() == doodle_config::doodle_json_extension;
          }) |
      ranges::views::transform([](const FSys::directory_entry &in_entry) -> FSys::path {
        return in_entry.path();
      }) |
      ranges::views::transform([this, in_handle](const FSys::path &in_path) -> ue4_import_data {
        auto l_f = export_file_info::read_file(in_path);
        ue4_import_data l_r{l_f, p_i->ue4_content_dir};
        l_r.import_file_save_dir = l_r.set_save_dir(in_handle);
        return l_r;
      }) |
      ranges::to_vector;
  /// 如果搜索不到就返回
  if (l_group.groups.empty())
    return;

  auto l_it = ranges::find_if(l_group.groups, [](const ue4_import_data &in_data) {
    return in_data.import_type == decltype(in_data.import_type)::camera;
  });
  if (l_it != l_group.groups.end()) {
    l_group.start_frame = l_it->start_frame;
    l_group.end_frame   = l_it->end_frame;
  } else if (!l_group.groups.empty()) {
    l_group.start_frame = l_group.groups.front().start_frame;
    l_group.end_frame   = l_group.groups.front().end_frame;
  }
  l_group.world_path = l_group.set_level_dir(in_handle, "_world");
  l_group.level_path = l_group.set_level_dir(in_handle, "_lev");

  FSys::path l_out_path = app::Get().options_->p_ue4outpath;
  if(!FSys::exists(l_out_path)){
    FSys::create_directories(l_out_path);
  }
  nlohmann::json l_json{};
  l_json = l_group;
  FSys::ofstream{l_out_path/core_set::getSet().get_uuid_str(".json_doodle")}
      << l_json.dump();
}
namespace ue4_widget_n {
ue4_import_data::ue4_import_data() = default;
ue4_import_data::ue4_import_data(const export_file_info &in_info,
                                 const FSys::path &in_ue4_content_dir)
    : import_file_path(in_info.file_path.generic_string()),
      import_file_save_dir(),
      import_type(in_info.export_type_),
      fbx_skeleton_file_name(),
      start_frame(in_info.start_frame),
      end_frame(in_info.end_frame) {
  fbx_skeleton_file_name = find_ue4_skin(in_info.ref_file, in_ue4_content_dir);
}
std::string ue4_import_data::find_ue4_skin(const FSys::path &in_ref_file,
                                           const FSys::path &in_ue4_content_dir) const {
  boost::contract::check l_ =
      boost::contract::public_function(this)
          .precondition([&]() {
            chick_true<doodle_error>(FSys::is_directory(in_ue4_content_dir), DOODLE_LOC,
                                     "无法找到ue4 content 文件夹");
          });

  std::string result{};

  switch (import_type) {
    case export_file_info::export_type::fbx: {
      auto l_fbx_skeleton = in_ref_file.stem();
      l_fbx_skeleton += "_Skeleton";
      auto l_path_it = ranges::find_if(
          ranges::make_subrange(
              FSys::recursive_directory_iterator{in_ue4_content_dir},
              FSys::recursive_directory_iterator{}),
          [&](const FSys::directory_entry &in_entry) -> bool {
            return in_entry.path().stem() == l_fbx_skeleton;
          });
      if (l_path_it != FSys::recursive_directory_iterator{}) {
        auto l_p = l_path_it->path().lexically_relative(in_ue4_content_dir);
        l_p      = FSys::path{doodle_config::ue4_game} / l_p;
        result   = l_p.generic_string();
      }
    } break;
    default:
      break;
  }

  return result;
}
std::string ue4_import_data::set_save_dir(const entt::handle &in_handle) const {
  std::string result{};
  boost::contract::check l_ =
      boost::contract::public_function(this)
          .precondition([&]() {
            chick_true<doodle_error>(in_handle, DOODLE_LOC,
                                     "无效的句柄");
          })
          .postcondition([&]() {
            chick_true<doodle_error>(!result.empty(), DOODLE_LOC,
                                     "设置路径为空");
          });
  auto l_p = FSys::path{doodle_config::ue4_game} /
             doodle_config::ue4_shot /
             fmt::format("{}", in_handle.get_or_emplace<episodes>()) /
             fmt::format("{}_{}",
                         in_handle.get_or_emplace<episodes>(),
                         in_handle.get_or_emplace<shot>()) /
             core_set::getSet().organization_name;
  return result = l_p.generic_string();
}
std::string ue4_import_group::set_level_dir(
    const entt::handle &in_handle,
    const std::string &in_e  ) const {
  std::string result{};
  boost::contract::check l_ =
      boost::contract::public_function(this)
          .precondition([&]() {
            chick_true<doodle_error>(in_handle, DOODLE_LOC,
                                     "无效的句柄");
          })
          .postcondition([&]() {
            chick_true<doodle_error>(!result.empty(), DOODLE_LOC,
                                     "设置路径为空");
          });
  auto l_p = FSys::path{doodle_config::ue4_game} /
             doodle_config::ue4_shot /
             fmt::format("{}", in_handle.get_or_emplace<episodes>()) /
             fmt::format("{}_{}",
                         in_handle.get_or_emplace<episodes>(),
                         in_handle.get_or_emplace<shot>()) /
             core_set::getSet().organization_name /
             fmt::format("{}_{}_{}_{}",
                         in_handle.get_or_emplace<episodes>(),
                         in_handle.get_or_emplace<shot>(),
                         core_set::getSet().organization_name,
                         in_e);
  return result = l_p.generic_string();
}
}  // namespace ue4_widget_n
}  // namespace doodle

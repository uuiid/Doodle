//
// Created by TD on 2022/4/1.
//

#include "ue4_widget.h"

#include <doodle_core/metadata/assets_file.h>
#include <doodle_core/metadata/episodes.h>
#include <doodle_core/metadata/shot.h>

#include <doodle_app/app/program_options.h>
#include <doodle_app/gui/base/ref_base.h>
#include <doodle_app/gui/open_file_dialog.h>
#include <doodle_app/lib_warp/imgui_warp.h>

#include <doodle_lib/app/doodle_main_app.h>
#include <doodle_lib/core/filesystem_extend.h>

#include <boost/contract.hpp>

namespace doodle::gui {
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
  gui::gui_cache<std::string> ue4_rig_regex{"正则修正"s, R"((\w+)_[Rr]ig.*)"s};
  gui::gui_cache<std::string> ue4_sk_fmt{"格式化结果"s, "SK_{}_Skeleton"s};
  gui::gui_cache<bool> quit_{"生成并退出"s, true};
  gui::gui_cache_name_id import_{"导入"s};
  gui::gui_cache_name_id open_file_dig{"选择"s};

  FSys::path ue4_content_dir{};
  std::vector<entt::handle> l_handle_temp{};
  std::string title_name_;
  FSys::path ue4_out_path{};
};

ue4_widget::ue4_widget() : p_i(std::make_unique<impl>()) { p_i->title_name_ = std::string{name}; }
ue4_widget::~ue4_widget() = default;

void ue4_widget::init() {
  auto &l_opt          = program_options::value();
  p_i->ue4_prj.data    = l_opt.p_ue4Project;
  p_i->ue4_prj.path    = l_opt.p_ue4Project;
  p_i->ue4_out_path    = l_opt.p_ue4outpath;
  p_i->ue4_content_dir = p_i->ue4_prj.path.parent_path() / doodle_config::ue4_content.data();
  g_reg()->ctx().emplace<ue4_widget &>(*this);
}

void ue4_widget::render() {
  if (ImGui::InputText(*p_i->ue4_prj.gui_name, &p_i->ue4_prj.data)) {
    p_i->ue4_prj.path    = p_i->ue4_prj.data;
    p_i->ue4_content_dir = p_i->ue4_prj.path.parent_path() / doodle_config::ue4_content.data();
  }
  ImGui::SameLine();
  if (ImGui::Button(*p_i->open_file_dig)) {
    auto l_file = std::make_shared<file_dialog>(file_dialog::dialog_args{}.add_filter(".uproject"s));
    l_file->async_read([this](const FSys::path &in) {
      this->p_i->ue4_prj.data = in.generic_string();
      this->p_i->ue4_prj.path = in;
      p_i->ue4_content_dir    = p_i->ue4_prj.path.parent_path() / doodle_config::ue4_content.data();
    });
    make_handle().emplace<gui_windows>(l_file);
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
  ImGui::InputText(*p_i->ue4_rig_regex.gui_name, &p_i->ue4_rig_regex.data);
  dear::HelpMarker{"使用正则表达式修正maya骨骼动画的引用文件名称"};
  ImGui::InputText(*p_i->ue4_sk_fmt.gui_name, &p_i->ue4_sk_fmt.data);
  dear::HelpMarker{"使用格式化方法,对正则表达式捕获组进行格式化,生成最终查找结果"};

  /// 开始导入
  if (ImGui::Button(*p_i->import_)) {
    this->import_ue4_prj();
    doodle_main_app::Get().stop_app();
  }
}
void ue4_widget::import_ue4_prj() {
  auto l_list = p_i->import_list_files.data |
                ranges::views::transform([](const auto &in_item) -> entt::handle { return in_item.handle_; }) |
                ranges::views::filter([](const entt::handle &in_handle) -> bool {
                  return in_handle && in_handle.any_of<assets_file>();
                }) |
                ranges::to_vector;
  /// \brief 导出前清除目录
  FSys::path l_out_path = p_i->ue4_out_path;
  if (FSys::exists(l_out_path))
    for (auto &l_p : FSys::directory_iterator{l_out_path}) {
      if (FSys::is_regular_file(l_p)) FSys::remove(l_p.path());
    }

  ranges::for_each(l_list, [this](const entt::handle &in_handle) {
    this->plan_file_path(in_handle.get<assets_file>().get_path_normal());
  });
}
void ue4_widget::accept_handle(const std::vector<entt::handle> &in_list) {
  p_i->import_list_files.data = in_list | ranges::views::filter([](const entt::handle &in_handle) -> bool {
                                  return in_handle && in_handle.any_of<assets_file>();
                                }) |
                                ranges::views::transform([](const entt::handle &in_handle) -> impl::ue4_file_gui {
                                  auto str = in_handle.get<assets_file>().name_attr();
                                  impl::ue4_file_gui l_gui{str, false};
                                  l_gui.handle_ = in_handle;
                                  return l_gui;
                                }) |
                                ranges::to_vector;
}
void ue4_widget::plan_file_path(const FSys::path &in_path) {
  for (auto &&h : p_i->l_handle_temp)
    if (h) h.destroy();

  using namespace ue4_widget_n;
  ue4_import_group l_group{};
  entt::handle l_h{};
  l_group.groups =
      ranges::make_subrange(FSys::directory_iterator{in_path}, FSys::directory_iterator{}) |
      ranges::views::filter([](const FSys::directory_entry &in_entry) -> bool {
        return in_entry.path().extension() == doodle_config::doodle_json_extension.data();
      }) |
      ranges::views::transform([](const FSys::directory_entry &in_entry) -> FSys::path { return in_entry.path(); }) |
      ranges::views::transform([this, &l_h](const FSys::path &in_path) -> ue4_import_data {
        l_h = export_file_info::read_file(in_path);
        ue4_import_data l_r{l_h.get<export_file_info>()};
        l_r.redirect_path(in_path);
        l_r.fbx_skeleton_file_name = l_r.find_ue4_skin(
            l_h.get<export_file_info>().ref_file, p_i->ue4_content_dir, p_i->ue4_rig_regex.data, p_i->ue4_sk_fmt.data
        );
        l_r.import_file_save_dir = l_r.set_save_dir(l_h);
        return l_r;
      }) |
      ranges::to_vector;
  p_i->l_handle_temp.push_back(l_h);
  /// 如果搜索不到就返回
  if (l_group.groups.empty()) return;

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
  l_group.world_path = l_group.set_level_dir(l_h, fmt::format("{}", core_set::get_set().organization_name.front()));
  l_group.level_path = l_group.set_level_dir(l_h, fmt::format("lev_{}", core_set::get_set().organization_name.front()));
  FSys::path l_out_path = p_i->ue4_out_path;
  if (!FSys::exists(l_out_path)) {
    FSys::create_directories(l_out_path);
  }
  nlohmann::json l_json{};
  l_json = l_group;
  FSys::ofstream{l_out_path / core_set::get_set().get_uuid_str(".json_doodle")} << l_json.dump();
}
const std::string &ue4_widget::title() const { return p_i->title_name_; }
namespace ue4_widget_n {
ue4_import_data::ue4_import_data() = default;
ue4_import_data::ue4_import_data(const export_file_info &in_info)
    : import_file_path(in_info.file_path.generic_string()),
      import_file_save_dir(),
      import_type(in_info.export_type_),
      fbx_skeleton_file_name(),
      start_frame(in_info.start_frame),
      end_frame(in_info.end_frame) {}
std::string ue4_import_data::find_ue4_skin(
    const FSys::path &in_ref_file, const FSys::path &in_ue4_content_dir, const std::string &in_regex,
    const std::string &in_fmt
) const {
  boost::contract::check l_ = boost::contract::public_function(this).precondition([&]() {
    FSys::is_directory(in_ue4_content_dir)
        ? void()
        : throw_exception(doodle_error{"无法找到ue4 content 文件夹 {}", in_ue4_content_dir});
    in_fmt.empty() ? throw_exception(doodle_error{"格式化字符串不可为空 {}", in_fmt}) : void();
    in_regex.empty() ? throw_exception(doodle_error{"正则表达式不可为空 "}) : void();
  });

  std::string result{};

  switch (import_type) {
    case export_file_info::export_type::fbx: {
      std::regex l_regex{in_regex};
      std::smatch l_smatch{};
      std::string l_token{};
      std::string l_stem{in_ref_file.stem().generic_string()};
      if (std::regex_match(l_stem, l_smatch, l_regex)) {
        if (l_smatch.size() == 2) {
          l_token = l_smatch[1].str();

          FSys::path l_fbx_skeleton{fmt::format(in_fmt, l_token)};
          auto l_path_it = ranges::find_if(
              ranges::make_subrange(
                  /// \brief 注意, 这里由于项目原因,需要遵循符号链接
                  FSys::recursive_directory_iterator{
                      in_ue4_content_dir, FSys::directory_options::follow_directory_symlink},
                  FSys::recursive_directory_iterator{}
              ),
              [&](const FSys::directory_entry &in_entry) -> bool { return in_entry.path().stem() == l_fbx_skeleton; }
          );
          if (l_path_it != FSys::recursive_directory_iterator{}) {
            auto l_p = l_path_it->path().lexically_relative(in_ue4_content_dir);
            l_p      = FSys::path{doodle_config::ue4_game.data()} / l_p.parent_path() / l_p.stem();
            result   = l_p.generic_string();
          }
        }
      }

    } break;
    default:
      break;
  }

  return result;
}
std::string ue4_import_data::set_save_dir(const entt::handle &in_handle) const {
  std::string result{};
  boost::contract::check l_ = boost::contract::public_function(this)
                                  .precondition([&]() {
                                    in_handle ? void() : throw_exception(doodle_error{"无效的句柄 {}"s, in_handle});
                                  })
                                  .postcondition([&]() {
                                    result.empty() ? throw_exception(doodle_error{"设置路径为空 {}"s, result}) : void();
                                  });
  auto l_p = FSys::path{doodle_config::ue4_game.data()} / doodle_config::ue4_shot.data() /
             fmt::format("ep{:04d}", in_handle.get_or_emplace<episodes>().p_episodes) /
             fmt::format(
                 "{}{:04d}_{:04d}{}", g_reg()->ctx().at<project>().short_str(),
                 in_handle.get_or_emplace<episodes>().p_episodes, in_handle.get_or_emplace<shot>().p_shot,
                 in_handle.get_or_emplace<shot>().p_shot_enum
             ) /
             core_set::get_set().organization_name;
  return result = l_p.generic_string();
}
std::string ue4_import_group::set_level_dir(const entt::handle &in_handle, const std::string &in_e) const {
  std::string result{};
  boost::contract::check l_ = boost::contract::public_function(this)
                                  .precondition([&]() {
                                    in_handle ? void() : throw_exception(doodle_error{"无效的句柄 {}"s, in_handle});
                                  })
                                  .postcondition([&]() {
                                    result.empty() ? throw_exception(doodle_error{"设置路径为空 {}"s, result}) : void();
                                  });
  auto l_p = FSys::path{doodle_config::ue4_game.data()} / doodle_config::ue4_shot.data() /
             fmt::format("ep{:04d}", in_handle.get_or_emplace<episodes>().p_episodes) /
             fmt::format(
                 "{}{:04d}_{:04d}{}", g_reg()->ctx().at<project>().short_str(),
                 in_handle.get_or_emplace<episodes>().p_episodes, in_handle.get_or_emplace<shot>().p_shot,
                 in_handle.get_or_emplace<shot>().p_shot_enum
             ) /
             fmt::format(
                 "{}{:04d}_sc{:04d}{}_{}", g_reg()->ctx().at<project>().short_str(),
                 in_handle.get_or_emplace<episodes>().p_episodes, in_handle.get_or_emplace<shot>().p_shot,
                 in_handle.get_or_emplace<shot>().p_shot_enum, in_e
             );
  return result = l_p.generic_string();
}

void to_json(nlohmann::json &j, const ue4_import_data &p) {
  j["import_file_path"]       = p.import_file_path;
  j["import_file_save_dir"]   = p.import_file_save_dir;
  j["import_type"]            = p.import_type;
  j["fbx_skeleton_file_name"] = p.fbx_skeleton_file_name;
  j["start_frame"]            = p.start_frame;
  j["end_frame"]              = p.end_frame;
}
void from_json(const nlohmann::json &j, ue4_import_data &p) {
  j.at("import_file_path").get_to(p.import_file_path);
  j.at("import_file_save_dir").get_to(p.import_file_save_dir);
  j.at("import_type").get_to(p.import_type);
  j.at("fbx_skeleton_file_name").get_to(p.fbx_skeleton_file_name);
  j.at("start_frame").get_to(p.start_frame);
  j.at("end_frame").get_to(p.end_frame);
}
void ue4_import_data::redirect_path(const FSys::path &in_path) {
  if (FSys::exists(import_file_path)) return;
  FSys::path l_p{import_file_path};
  import_file_path = (in_path.parent_path() / l_p.filename()).generic_string();
}
void to_json(nlohmann::json &j, const ue4_import_group &p) {
  j["start_frame"] = p.start_frame;
  j["end_frame"]   = p.end_frame;
  j["world_path"]  = p.world_path;
  j["level_path"]  = p.level_path;
  j["groups"]      = p.groups;
}
void from_json(const nlohmann::json &j, ue4_import_group &p) {
  j.at("start_frame").get_to(p.start_frame);
  j.at("end_frame").get_to(p.end_frame);
  j.at("world_path").get_to(p.world_path);
  j.at("level_path").get_to(p.level_path);
  j.at("groups").get_to(p.groups);
}

}  // namespace ue4_widget_n
}  // namespace doodle::gui

//
// Created by TD on 2021/8/4.
//

#include "tool_box_menu_factory.h"

#include <Exception/Exception.h>
#include <Gui/action/actn_tool_box.h>
#include <Gui/progress.h>
#include <Metadata/Episodes.h>
#include <Metadata/Metadata.h>
#include <Metadata/Project.h>
#include <Metadata/Shot.h>

#include <nana/gui/filebox.hpp>
#include <nana/gui/msgbox.hpp>

namespace doodle {

tool_box_menu_factory::tool_box_menu_factory(nana::window in_window)
    : p_window(in_window),
      p_list() {
}

void tool_box_menu_factory::create_menu() {
  auto k_export_maya = std::make_shared<toolbox::actn_export_maya>();
  p_list.push_back(k_export_maya);
  auto k_get_paths = [this]() {
    nana::filebox k_filebox{p_window, true};
    k_filebox.allow_multi_select(true);

    toolbox::actn_export_maya::arg k_arg{};
    k_arg.date      = k_filebox();
    k_arg.is_cancel = k_arg.date.empty();
    return k_arg;
  };
  k_export_maya->sig_get_arg.connect(k_get_paths);

  auto k_create_video = std::make_shared<toolbox::actn_create_video>();
  p_list.push_back(k_create_video);
  k_create_video->sig_get_arg.connect([this]() {
    nana::folderbox k_filebox{p_window, FSys::current_path(), "选择目录"};
    k_filebox.allow_multi_select(true);

    toolbox::actn_export_maya::arg k_arg{};
    k_arg.date      = k_filebox();
    k_arg.is_cancel = k_arg.date.empty();
    return k_arg;
  });

  auto k_connect_video = std::make_shared<toolbox::actn_connect_video>();
  p_list.push_back(k_connect_video);
  k_connect_video->sig_get_arg.connect(k_get_paths);

  auto k_ue4 = std::make_shared<toolbox::actn_ue4_shot_episodes>();
  p_list.push_back(k_ue4);
  k_ue4->sig_get_arg.connect([this]() {
    nana::filebox k_filebox{p_window, true};
    k_filebox.allow_multi_select(false);

    nana::inputbox k_input_box{p_window, "镜头范围"};
    nana::inputbox::text k_prj{"项目名称: "};
    nana::inputbox::integer k_eps{"集数", 1, 1, 9999, 1};
    nana::inputbox::integer k_b{"开始", 1, 1, 9999, 1};
    nana::inputbox::integer k_e{"结束", 1, 1, 9999, 1};

    k_input_box.show_modal(k_prj, k_b, k_e);
    toolbox::actn_ue4_shot_episodes::arg k_arg{};
    const auto k_range = k_e.value() - k_b.value();
    const auto k_b_int = k_b.value();

    k_arg.project = std::make_shared<Project>("D:/", k_prj.value());

    k_arg.epsiodes = std::make_shared<Episodes>(k_arg.project, k_eps.value());
    k_arg.project->child_item.push_back_sig(k_arg.epsiodes->shared_from_this());
    std::generate_n(std::back_inserter(k_arg.shot_list),
                    k_range > 0 ? k_range : 0,
                    [n = k_b_int, &k_arg]() mutable {
                      auto k_shot = std::make_shared<Shot>(k_arg.epsiodes, n);
                      k_arg.epsiodes->child_item.push_back_sig(k_shot->shared_from_this());
                      ++n;
                      return k_shot;
                    });

    auto k_paths    = k_filebox();
    k_arg.date      = k_paths.empty() ? FSys::path{} : k_paths.front();
    k_arg.is_cancel = k_arg.date.empty() || k_range < 0;

    return k_arg;
  });

  auto k_qcloth = std::make_shared<toolbox::actn_qcloth_sim_export>();
  p_list.push_back(k_qcloth);
  k_qcloth->sig_get_arg.connect([this]() {
    nana::filebox k_filebox{p_window, true};
    k_filebox.allow_multi_select(true);

    toolbox::actn_qcloth_sim_export::arg k_arg{};
    k_arg.date      = k_filebox();
    k_arg.is_cancel = k_arg.date.empty();

    nana::folderbox k_folder_box{p_window, FSys::current_path()};

    k_folder_box.allow_multi_select(false);
    auto k_paths = k_folder_box();
    k_arg.is_cancel &= k_paths.empty();
    if (k_arg.is_cancel)
      return k_arg;
    if (!k_paths.empty())
      k_arg.qcloth_assets_path = k_paths.front();
    else
      k_arg.is_cancel = true;
    return k_arg;
  });

  auto k_import_ue4 = std::make_shared<toolbox::actn_ue4_import_files>();
  p_list.push_back(k_import_ue4);
  k_import_ue4->sig_get_arg.connect([this]() {
    toolbox::actn_ue4_import_files::arg k_arg{};

    {
      nana::filebox k_filebox{p_window, true};
      k_filebox.add_filter("ue4 project", "*.uproject");
      k_filebox.allow_multi_select(false);

      auto k_pas = k_filebox();
      if (!k_pas.empty())
        k_arg.ue4_project = k_pas.front();
      k_arg.is_cancel = k_pas.empty();
    }

    {
      nana::filebox k_filebox{p_window, true};
      k_filebox.add_filter("fbx file and abc file", "*.fbx;*.abc");
      k_filebox.allow_multi_select(true);

      k_arg.date      = k_filebox();
      k_arg.is_cancel = k_arg.date.empty();
    }

    return k_arg;
  });
}
void tool_box_menu_factory::operator()(nana::menu& in_menu) {
  create_menu();
  auto k_f = shared_from_this();
  for (auto& k_i : p_list) {
    if (k_i)
      in_menu.append(
          k_i->class_name(),
          [k_i, k_f](const nana::menu::item_proxy&) {
            try {
              (*k_i)();
              if (k_i->is_async()) {
                auto k_long = k_i->get_long_term_signal();
                if (k_long)
                  progress::create_progress(k_f->p_window, k_long, "结果");
              }
            } catch (DoodleError& error) {
              DOODLE_LOG_WARN(error.what())
              nana::msgbox k_msgbox{k_f->p_window, error.what()};
              k_msgbox.show();
            }
          });
    else
      in_menu.append_splitter();
  }
}
}  // namespace doodle

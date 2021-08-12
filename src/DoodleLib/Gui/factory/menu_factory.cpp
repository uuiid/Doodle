//
// Created by TD on 2021/6/28.
//

#include "menu_factory.h"

#include <Gui/action/action_import.h>
#include <Gui/progress.h>
#include <Metadata/AssetsFile.h>
#include <Metadata/Metadata_cpp.h>
#include <threadPool/long_term.h>

#include <nana/gui/filebox.hpp>

namespace doodle {

menu_factory_base::menu_factory_base(nana::window in_window)
    : p_window(in_window),
      p_set(CoreSet::getSet()),
      p_action(),
      p_metadata(),
      p_parent() {
}

void menu_factory_base::set_metadate(const MetadataPtr& in_ptr, const MetadataPtr& in_parent) {
  p_metadata = in_ptr;
  p_parent   = in_parent;
  p_action.clear();
}

void menu_factory_base::operator()(nana::menu& in_menu) {
  auto k_f = shared_from_this();
  for (auto& k_i : p_action) {
    if (k_i)
      in_menu.append(
          k_i->class_name(),
          [k_i, k_f](const nana::menu::item_proxy&) {
            try {
              (*k_i)(k_f->p_metadata, k_f->p_parent);
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

menu_factory::menu_factory(nana::window in_window)
    : menu_factory_base(in_window) {
}

void menu_factory::create_menu(const ProjectPtr& in_ptr) {}
void menu_factory::create_menu(const AssetsPtr& in_ptr) {}
void menu_factory::create_menu(const EpisodesPtr& in_ptr) {}
void menu_factory::create_menu(const ShotPtr& in_ptr) {}
void menu_factory::create_menu(const AssetsFilePtr& in_ptr) {}
void menu_factory::create_menu(const season_ptr& in_ptr) {}

void menu_factory::delete_project() {
  if (!p_metadata->hasChild()) {
    p_action.emplace_back(action_ptr{});
    p_action.emplace_back(std::make_shared<actn_delete_project>());
  }
}
void menu_factory::modify_project_set_path() {
  auto k_rp = std::make_shared<actn_setpath_project>();
  p_action.emplace_back(k_rp);
  k_rp->sig_get_arg.connect([this]() {
    nana::folderbox mes{
        p_window};
    mes.allow_multi_select(false).title("选择项目根目录: ");

    auto k_e = mes();
    actn_setpath_project::arg k_arg{};
    k_arg.date = k_e.at(0);
    return k_arg;
  });
}
void menu_factory::modify_project_rename() {
  auto k_rm = std::make_shared<actn_rename_project>();
  p_action.push_back(k_rm);
  k_rm->sig_get_arg.connect([this]() {
    nana::inputbox mess{p_window, "项目名称: "};
    nana::inputbox::text name{"项目名称: "};
    mess.show_modal(name);
    auto str = name.value();

    actn_rename_project::arg k_arg{};
    k_arg.date = str;
    return k_arg;
  });
}
void menu_factory::modify_assets_set_name() {
  auto k_s_ass = std::make_shared<actn_assets_setname>();
  p_action.push_back(k_s_ass);
  k_s_ass->sig_get_arg.connect([this]() {
    nana::inputbox msg{p_window, "创建: "};
    nana::inputbox::text name{"名称: "};
    msg.show_modal(name);
    return actn_assets_setname::arg{name.value()};
  });
}
void menu_factory::modify_episode() {
  auto k_s_eps = std::make_shared<actn_episode_set>();
  p_action.push_back(k_s_eps);
  k_s_eps->sig_get_arg.connect([this]() {
    nana::inputbox msg{p_window, "修改: "};
    nana::inputbox::integer k_i{"集数: ", 0, 0, 9999, 1};
    msg.show_modal(k_i);
    return actn_episode_set::arg{k_i.value()};
  });
}
void menu_factory::modify_shot_ab() {
  auto k_s_shab = std::make_shared<actn_shotab_set>();
  p_action.push_back(k_s_shab);
  k_s_shab->sig_get_arg.connect([this]() {
    nana::inputbox msg{p_window, "修改: "};
    auto k_s_v = magic_enum::enum_names<Shot::ShotAbEnum>();
    nana::inputbox::text k_text{"ab镜头号:", std::vector<std::string>{k_s_v.begin(), k_s_v.end()}};
    msg.show_modal(k_text);
    return actn_shotab_set::arg{k_text.value()};
  });
}
void menu_factory::modify_shot_int() {
  auto k_s_sh = std::make_shared<actn_shot_set>();
  p_action.push_back(k_s_sh);

  k_s_sh->sig_get_arg.connect([this]() {
    nana::inputbox msg{p_window, "修改: "};
    nana::inputbox::integer k_i{"镜头号: ", 0, 0, 9999, 1};
    msg.show_modal(k_i);
    return actn_shot_set::arg{k_i.value()};
  });
}
void menu_factory::delete_assets_attr() { p_action.emplace_back(std::make_shared<actn_assfile_delete>()); }

void menu_factory::down_file() {
  auto p_ptr = std::dynamic_pointer_cast<AssetsFile>(p_metadata);

  if (!p_ptr->getPathFile().empty()) {
    auto k_down = std::make_shared<actn_down_paths>();
    p_action.push_back(k_down);
    k_down->sig_get_arg.connect([this]() {
      nana::folderbox k_folderbox{p_window, FSys::current_path()};
      k_folderbox.allow_multi_select(false);
      auto k_paths = k_folderbox();
      actn_down_paths::arg_ arg{k_paths.front()};
      if (k_paths.empty())
        arg.is_cancel = true;
      return arg;
    });
  }
}
void menu_factory::modify_attr_set_time() {
  auto k_s_t = std::make_shared<actn_assfile_datetime>();
  p_action.push_back(k_s_t);
  k_s_t->sig_get_arg.connect([this]() {
    nana::inputbox msg{p_window, "修改: "};
    //    nana::inputbox::date k_date{"时间: "};
    auto time = std::dynamic_pointer_cast<AssetsFile>(p_metadata)->getTime();
    nana::inputbox::integer k_year{"年", time->get_year(), 1999, 2999, 1};
    nana::inputbox::integer k_month{"月", time->get_month(), 1, 12, 1};
    nana::inputbox::integer k_day{"天", time->get_day(), 1, 31, 1};
    nana::inputbox::integer k_hours{"时", time->get_hour(), 0, 23, 1};
    nana::inputbox::integer k_minutes{"分", time->get_minutes(), 0, 59, 1};
    nana::inputbox::integer k_seconds{"秒", time->get_second(), 0, 59, 1};

    msg.show_modal(k_year, k_month, k_day, k_hours, k_minutes, k_seconds);

    time->set_year(k_year.value());
    time->set_month(k_month.value());
    time->set_day(k_day.value());
    time->set_hour(k_hours.value());
    time->set_minutes(k_minutes.value());
    time->set_second(k_seconds.value());
    return actn_assfile_datetime::arg{time};
  });
}
void menu_factory::modify_attr_add_com() {
  auto k_s_c = std::make_shared<actn_assfile_add_com>();
  p_action.push_back(k_s_c);
  k_s_c->sig_get_arg.connect([this]() {
    nana::inputbox msg{p_window, "评论: "};
    nana::inputbox::text name{"评论: "};
    msg.show_modal(name);
    return actn_assfile_add_com::arg{name.value()};
  });
}
void menu_factory::create_assets() {
  auto k_c_ass = std::make_shared<actn_assets_create>();
  p_action.push_back(k_c_ass);

  k_c_ass->sig_get_arg.connect([this]() {
    nana::inputbox msg{p_window, "创建: "};
    nana::inputbox::text name{"名称: "};
    msg.show_modal(name);
    return actn_assets_create::arg{name.value()};
  });
}
void menu_factory::create_episodes() {
  auto k_c_eps = std::make_shared<actn_episode_create>();
  p_action.push_back(k_c_eps);
  k_c_eps->sig_get_arg.connect([this]() {
    nana::inputbox msg{p_window, "创建: "};
    nana::inputbox::integer k_i{"集数: ", 0, 0, 9999, 1};
    msg.show_modal(k_i);
    return actn_episode_create::arg{k_i.value()};
  });
}
void menu_factory::create_shot() {
  auto k_c_shot = std::make_shared<actn_shot_create>();
  p_action.push_back(k_c_shot);

  k_c_shot->sig_get_arg.connect([this]() {
    nana::inputbox msg{p_window, "创建: "};
    nana::inputbox::integer k_i{"镜头号: ", 0, 0, 9999, 1};
    msg.show_modal(k_i);
    return actn_shot_create::arg{k_i.value()};
  });
}
void menu_factory::create_assets_file() {
  auto k_c_f = std::make_shared<actn_assfile_create>();
  p_action.push_back(k_c_f);
  k_c_f->sig_get_arg.connect([this]() {
    return actn_assfile_create::arg{this->p_parent->showStr()};
  });
}
void menu_factory::create_prj() {
  auto k_create = std::make_shared<actn_create_project>();
  p_action.push_back(k_create);

  k_create->sig_get_arg.connect([this]() {
    nana::folderbox mes{
        p_window, FSys::current_path()};
    mes.allow_multi_select(false).title("选择项目根目录: ");

    auto k_e = mes();
    if (k_e.empty())
      return actn_create_project::arg{};

    nana::inputbox mess{p_window, "项目名称: "};

    nana::inputbox::text name{"项目名称: "};
    mess.show_modal(name);
    auto str = name.value();
    if (str.empty())
      return actn_create_project::arg{};
    actn_create_project::arg k_arg{};
    k_arg.prj_path = k_e.at(0);
    k_arg.name     = str;

    return k_arg;
  });
}
void menu_factory::delete_assets() {
  if (!p_metadata)
    return;

  if (!p_metadata->hasChild()) {
    p_action.emplace_back(action_ptr{});
    auto k_d_ = p_action.emplace_back(std::make_shared<actn_assets_delete>());
  }
}
void menu_factory::create_assets_file_up_data() {
  auto k_up_folder = std::make_shared<actn_create_ass_up_paths>();
  p_action.push_back(k_up_folder);

  k_up_folder->sig_get_arg.connect([this]() {
    actn_up_paths::arg_ k_arg{};
    nana::filebox k_file{this->p_window, true};
    k_file.allow_multi_select(true);
    k_arg.date      = k_file.show();
    k_arg.is_cancel = k_arg.date.empty();
    return k_arg;
  });
}
void menu_factory::create_assets_file_video_up() {
  auto k_i_anf_up = std::make_shared<actn_image_to_move_up>();
  k_i_anf_up->sig_get_arg.connect([this, k_i_anf_up]() {
    actn_image_to_move_up::arg_ k_arg{};
    nana::folderbox k_file{this->p_window, FSys::current_path()};
    k_file.allow_multi_select(false);
    k_arg.image_list = k_file.show();
    k_arg.out_file   = CoreSet::getSet().getCacheRoot("imaeg_to_move");
    k_arg.is_cancel  = !k_i_anf_up->is_accept(k_arg);
    return k_arg;
  });

  p_action.push_back(k_i_anf_up);
}

void menu_factory::create_assets_file_export_maya_up() {
  auto k_ex = std::make_shared<actn_maya_export>();
  if (!p_metadata)
    return;
  auto k_files = std::dynamic_pointer_cast<AssetsFile>(p_metadata)
                     ->getPathFile();
  if (k_files.empty())
    return;

  auto path = k_files.front()->getServerPath();
  actn_maya_export::arg arg{};
  arg.date = path;
  if (k_ex->is_accept(arg)) {
    p_action.push_back(k_ex);
    k_ex->sig_get_arg.connect([arg]() {
      return arg;
    });
  }
}

void menu_factory::create_assets_file_batch_video_up() {
}

void menu_factory::create_assets_file_batch_export_maya_up() {
}

void menu_factory::create_ue4_Sequencer() {
}

void menu_factory::modify_assets_file_up_data() {
  auto k_up_folder = std::make_shared<actn_up_paths>();
  p_action.push_back(k_up_folder);

  k_up_folder->sig_get_arg.connect([this]() {
    actn_up_paths::arg_ k_arg{};
    nana::filebox k_file{this->p_window, true};
    k_file.allow_multi_select(true);
    k_arg.date      = k_file.show();
    k_arg.is_cancel = k_arg.date.empty();
    return k_arg;
  });
}
void menu_factory::show_assets_file_attr() {
  auto k_show = std::make_shared<actn_assdile_attr_show>();
  p_action.push_back(k_show);
  k_show->sig_get_arg.connect([this]() {
    auto k_item = std::dynamic_pointer_cast<AssetsFile>(p_metadata);
    std::string k_com{};
    std::string k_path{};
    for (const auto& k_i : k_item->getComment()) {
      k_com += fmt::format("{}\n", k_i->getComment());
    }
    for (const auto& k_i : k_item->getPathFile()) {
      k_path += fmt::format("{}\n", k_i->str());
    }
    auto str = fmt::format(R"(详细信息：
名称： {}
版本： {}
制作者： {}
部门: {}
制作时间： {}
评论：
{}
路径:
{}
)",
                           k_item->showStr(),                                 //名称
                           k_item->getVersionStr(),                           //版本
                           k_item->getUser(),                                 //制作者
                           magic_enum::enum_name<>(k_item->getDepartment()),  // 部门
                           k_item->getTime()->showStr(),                      //制作时间
                           k_com,                                             //评论
                           k_path                                             // 路径
    );

    nana::msgbox k_msgbox{p_window, "assets attr"};
    k_msgbox << str;
    k_msgbox.show();
    return actn_assdile_attr_show::arg{};
  });
}
void menu_factory::export_excel() {
  auto k_ex = std::make_shared<actn_export_excel>();
  p_action.emplace_back(k_ex);

  k_ex->sig_get_arg.connect([this]() {
    actn_export_excel::arg k_arg{};
    nana::inputbox msg{p_window, "选择时间: "};
    auto k_time_b = std::make_shared<TimeDuration>();
    nana::inputbox::integer k_year{"年", k_time_b->get_year(), 1999, 2999, 1};
    nana::inputbox::integer k_men{"月", k_time_b->get_month(), 1, 12, 1};
    msg.show_modal(k_year, k_men);
    k_time_b->set_year(k_year.value());
    k_time_b->set_month(k_men.value());
    k_time_b->set_day(1);
    k_time_b->set_hour(0);
    k_time_b->set_minutes(0);
    k_time_b->set_second(0);

    auto k_time_end = std::make_shared<TimeDuration>(k_time_b->getUTCTime());
    k_time_end->set_day((std::uint32_t)chrono::year_month_day_last{chrono::year{k_time_b->get_year()},
                                                                   chrono::month_day_last{
                                                                       chrono::month{k_time_b->get_month()}}}
                            .day());
    k_time_end->set_hour(23);
    k_time_end->set_minutes(59);
    k_time_end->set_second(59);
    k_arg.p_time_range = std::make_pair(k_time_b, k_time_end);

    nana::folderbox k_folder{p_window, FSys::current_path()};
    k_folder.allow_multi_select(false);
    k_arg.date = k_folder.show().front();

    k_arg.is_cancel = k_arg.date == FSys::current_path();
    return k_arg;
  });
}

void menu_factory::create_season() {
  auto item = std::make_shared<actn_season_create>();
  p_action.push_back(item);
  item->sig_get_arg.connect([this]() {
    actn_season_create::arg arg{};
    nana::inputbox k_inputbox{p_window, "创建季数"};
    nana::inputbox::integer k_int{"季数", 1, 1, 999, 1};
    arg.is_cancel = !k_inputbox.show_modal(k_int);
    arg.date      = k_int.value();
    return arg;
  });
}
void menu_factory::modify_season() {
  auto item = std::make_shared<actn_season_set>();
  item->sig_get_arg.connect([this]() {
    actn_season_set::arg arg{};
    nana::inputbox k_inputbox{p_window, "修改季数"};
    nana::inputbox::integer k_int{"季数", 1, 1, 999, 1};
    arg.is_cancel = !k_inputbox.show_modal(k_int);
    arg.date      = k_int.value();
    return arg;
  });
}
void dragdrop_menu_factory::create_menu(const ProjectPtr& in_ptr) {
}
void dragdrop_menu_factory::create_menu(const AssetsPtr& in_ptr) {
  drop_menu();
}
void dragdrop_menu_factory::create_menu(const EpisodesPtr& in_ptr) {
  drop_menu();
}
void dragdrop_menu_factory::create_menu(const ShotPtr& in_ptr) {
  drop_menu();
}
void dragdrop_menu_factory::create_menu(const AssetsFilePtr& in_ptr) {
  drop_menu();
}
void dragdrop_menu_factory::drop_menu() {
  if (p_paths.empty())
    return;
  if (!p_metadata)
    return;

  //有任何一个路径不存在之际返回
  for (const auto& k_p : p_paths) {
    if (!FSys::exists(k_p))
      return;
  }

  auto k_up_folder = std::make_shared<actn_up_paths>();
  p_action.push_back(k_up_folder);
  k_up_folder->sig_get_arg.connect([this]() {
    return actn_up_paths::arg_{p_paths};
  });
  p_metadata.reset();

  if (p_paths.size() == 1) {
    auto k_path = p_paths.front();

    if (FSys::is_directory(k_path)) {  ///这是一个目录 如果可以转换为视频的话直接转换为视频
      auto k_image = std::make_shared<actn_image_to_movie>();
      actn_image_to_movie::arg_ k_arg{};
      k_arg.image_list = {k_path};
      if (k_image->is_accept(k_arg)) {
        /// 直接转换动画但是并不上传
        k_image->sig_get_arg.connect([this]() {
          actn_image_to_movie::arg_ k_arg{};
          k_arg.image_list = p_paths;
          k_arg.out_file   = CoreSet::getSet().getCacheRoot("imaeg_to_move");
          return k_arg;
        });

        p_action.push_back(k_image);
      }

    } else if (FSys::is_regular_file(k_path)) {
    } else {
      return;
    }

  } else {
  }
  p_action.emplace_back(action_ptr{});
  p_action.emplace_back(std::make_shared<actn_null>());
}
void dragdrop_menu_factory::create_image_and_up() {
  auto k_i_anf_up = std::make_shared<actn_image_to_move_up>();
  k_i_anf_up->sig_get_arg.connect([this]() {
    actn_image_to_move_up::arg_ k_arg{};
    k_arg.image_list = p_paths;
    k_arg.out_file   = CoreSet::getSet().getCacheRoot("imaeg_to_move");
    return k_arg;
  });

  p_action.push_back(k_i_anf_up);
}
void dragdrop_menu_factory::set_drop_file(const std::vector<FSys::path>& in_path) {
  p_paths = in_path;
}
void dragdrop_menu_factory::create_menu(const season_ptr& in_ptr) {
}

void menu_factory_project::create_menu(const ProjectPtr& in_ptr) {
  create_prj();
  export_excel();
  if (p_metadata) {  /// 首先测试是否选中prj 这种情况下显示所有
    //----------------------------------------------------------------

    p_action.emplace_back(action_ptr{});
    modify_project_rename();
    modify_project_set_path();
    delete_project();
  }
}
menu_factory_project::menu_factory_project(nana::window in_window) : menu_factory(in_window) {
}
menu_factory_assets::menu_factory_assets(nana::window in_window) : menu_factory(in_window) {
}
void menu_factory_assets::create_menu(const ProjectPtr& in_ptr) {
  if (p_parent) {
    create_assets();
    create_episodes();
    create_shot();
    create_season();
  }
}
void menu_factory_assets::create_menu(const AssetsPtr& in_ptr) {
  if (p_parent) {
    create_assets();
    create_episodes();
    create_shot();
    create_season();
  }
  if (p_metadata) {
    p_action.emplace_back(action_ptr{});
    modify_assets_set_name();
    delete_assets();
  }
}
void menu_factory_assets::create_menu(const EpisodesPtr& in_ptr) {
  if (p_parent) {
    create_assets();
    create_episodes();
    create_shot();
    create_season();
  }
  if (p_metadata) {
    p_action.emplace_back(action_ptr{});
    modify_episode();
    delete_assets();
  }
}
void menu_factory_assets::create_menu(const ShotPtr& in_ptr) {
  if (p_parent) {
    create_assets();
    create_episodes();
    create_shot();
    create_season();
  }
  if (p_metadata) {
    p_action.emplace_back(action_ptr{});
    modify_shot_int();
    modify_shot_ab();
    delete_assets();
  }
}

void menu_factory_assets::create_menu(const season_ptr& in_ptr) {
  if (p_parent) {
    create_assets();
    create_episodes();
    create_shot();
    create_season();
  }
  if (p_metadata) {
    p_action.emplace_back(action_ptr{});
    modify_season();
    delete_assets();
  }
}

menu_factory_assets_attr::menu_factory_assets_attr(nana::window in_window)
    : menu_factory(in_window) {
}
void menu_factory_assets_attr::create_menu(const AssetsPtr& in_ptr) {
  if (p_parent) {
    create_assets_file();
    create_assets_file_up_data();
    create_assets_file_video_up();
  }
}
void menu_factory_assets_attr::create_menu(const EpisodesPtr& in_ptr) {
  if (p_parent) {
    create_assets_file();
    create_assets_file_up_data();
    create_assets_file_video_up();
  }
}
void menu_factory_assets_attr::create_menu(const ShotPtr& in_ptr) {
  if (p_parent) {
    create_assets_file();
    create_assets_file_up_data();
    create_assets_file_video_up();
  }
}
void menu_factory_assets_attr::create_menu(const AssetsFilePtr& in_ptr) {
  if (p_parent) {
    create_assets_file();
    create_assets_file_up_data();
    create_assets_file_video_up();
  }
  if (p_metadata) {
    p_action.emplace_back(action_ptr{});
    show_assets_file_attr();
    modify_attr_add_com();
    modify_attr_set_time();
    modify_assets_file_up_data();

    p_action.push_back(action_ptr{});
    down_file();

    p_action.push_back(action_ptr{});
    delete_assets_attr();
  }
}

void menu_factory_assets_attr::create_menu(const season_ptr& in_ptr) {
  if (p_parent) {
    create_assets_file();
    create_assets_file_up_data();
    create_assets_file_video_up();
  }
}
}  // namespace doodle

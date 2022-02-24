
//
// Created by TD on 2021/9/23.
//

#include "edit_widgets.h"

#include <doodle_lib/gui/action/command.h>
#include <doodle_lib/gui/action/command_ue4.h>
#include <doodle_lib/gui/action/command_video.h>
#include <doodle_lib/metadata/metadata.h>
#include <doodle_lib/core/core_sig.h>
#include <doodle_lib/gui/widgets/assets_file_widgets.h>
#include <doodle_lib/metadata/assets_file.h>
#include <doodle_lib/gui/widgets/drag_widget.h>
#include <doodle_lib/long_task/drop_file_data.h>
#include <doodle_lib/metadata/metadata_cpp.h>
#include <doodle_lib/gui/gui_ref/ref_base.h>
#include <doodle_lib/gui/gui_ref/database_edit.h>
#include <doodle_lib/metadata/image_icon.h>
#include <doodle_lib/core/image_loader.h>

namespace doodle {
/**
 * @brief 季数编辑
 *
 */
class season_edit : public gui::edit_interface {
 public:
  std::int32_t p_season;

  void init_(const entt::handle &in) {
    if (in.any_of<season>())
      p_season = in.get<season>().get_season();
    else
      p_season = 1;
  }
  void render(const entt::handle &in) {
    if (imgui::InputInt("季数", &p_season, 1, 9999))
      set_modify(true);
  }
  void save_(const entt::handle &in) const {
    in.emplace_or_replace<season>(p_season);
  }
};
class episodes_edit : public gui::edit_interface {
 public:
  std::int32_t p_eps{1};

  void init_(const entt::handle &in) {
    if (in.all_of<episodes>())
      p_eps = in.get<episodes>().p_episodes;
    else
      p_eps = 1;
  }
  void render(const entt::handle &in) {
    if (imgui::InputInt("集数", &p_eps, 1, 9999))
      set_modify(true);
    ;
  }
  void save_(const entt::handle &in) const {
    in.emplace_or_replace<episodes>(p_eps);
  }
};
class shot_edit : public gui::edit_interface {
 public:
  std::int32_t p_shot;
  std::string p_shot_ab_str;

  void init_(const entt::handle &in) {
    if (in.all_of<shot>()) {
      auto &l_s     = in.get<shot>();
      p_shot        = l_s.get_shot();
      p_shot_ab_str = l_s.get_shot_ab();
    } else {
      p_shot        = 1;
      p_shot_ab_str = "None";
    }
  }
  void render(const entt::handle &in) {
    if (imgui::InputInt("镜头", &p_shot, 1, 9999)) set_modify(true);

    dear::Combo{"ab镜头", p_shot_ab_str.c_str()} && [this]() {
      static auto shot_enum{magic_enum::enum_names<shot::shot_ab_enum>()};
      for (auto &i : shot_enum) {
        if (imgui::Selectable(i.data(), i == p_shot_ab_str)) {
          p_shot_ab_str = i;
          set_modify(true);
        }
      }
    };
  }
  void save_(const entt::handle &in) const {
    in.emplace_or_replace<shot>(p_shot, p_shot_ab_str);
  }
};
class assets_file_edit : public gui::edit_interface {
 public:
  std::string p_path_cache;

  void init_(const entt::handle &in) {
    if (in.all_of<assets_file>()) {
      p_path_cache = in.get<assets_file>().path.generic_string();
    } else {
      p_path_cache = g_reg()->ctx<project>().p_path.generic_string();
    }
  }
  void render(const entt::handle &in) {
    if (ImGui::InputText("路径", &p_path_cache)) set_modify(true);
  }
  void save_(const entt::handle &in) const {
    in.emplace_or_replace<assets_file>().path = p_path_cache;
  }
};

class time_edit : public gui::edit_interface {
 public:
  gui::gui_cache<std::int32_t> p_year;
  gui::gui_cache<std::int32_t> p_month;
  gui::gui_cache<std::int32_t> p_day;

  gui::gui_cache<std::int32_t> p_hours;
  gui::gui_cache<std::int32_t> p_minutes;
  gui::gui_cache<std::int32_t> p_seconds;

 public:
  time_edit()
      : p_year("年"s, 0),
        p_month("月"s, 0),
        p_day("天"s, 0),
        p_hours("时"s, 0),
        p_minutes("分"s, 0),
        p_seconds("秒"s, 0) {}
  void init_(const entt::handle &in) override {
    std::tie(p_year,
             p_month,
             p_day,
             p_hours,
             p_minutes,
             p_seconds) = in.get_or_emplace<time_point_wrap>().compose();
  }

  void render(const entt::handle &in) override {
    if (ImGui::SliderInt(*p_year.gui_name, &p_year.data, 2020, 2050))
      set_modify(true);
    if (ImGui::SliderInt(*p_month.gui_name, &p_month.data, 1, 12))
      set_modify(true);
    if (ImGui::SliderInt(*p_day.gui_name, &p_day.data, 0, 31))
      set_modify(true);
    if (ImGui::SliderInt(*p_hours.gui_name, &p_hours.data, 0, 23))
      set_modify(true);
    if (ImGui::SliderInt(*p_minutes.gui_name, &p_minutes.data, 0, 59))
      set_modify(true);
    if (ImGui::SliderInt(*p_seconds.gui_name, &p_seconds.data, 0, 59))
      set_modify(true);
  }
  void save_(const entt::handle &in) const override {
    in.emplace_or_replace<time_point_wrap>(p_year,
                                           p_month,
                                           p_day,
                                           p_hours,
                                           p_minutes,
                                           p_seconds);
  }
};

namespace gui {

class add_assets_for_file : public base_render {
  void add_time(const entt::handle &in_handle, const FSys::path &in_path) {
    if (FSys::exists(in_path)) {
      in_handle.emplace_or_replace<time_point_wrap>(FSys::last_write_time_point(in_path));
    }
  };

  void find_icon(const entt::handle &in_handle, const FSys::path &in_path) {
    image_loader l_image_load{};
    auto k_imghe_path = ranges::find_if(
        ranges::make_subrange(
            FSys::directory_iterator{
                is_directory(in_path) ? in_path : in_path.parent_path()},
            FSys::directory_iterator{}),
        [](const FSys::path &in_file) {
          auto &&l_ext = in_file.extension();
          return l_ext == ".png" || l_ext == ".jpg";
        });
    if (k_imghe_path != FSys::directory_iterator{}) {
      l_image_load.save(in_handle, k_imghe_path->path());
    }
  };

  void add_assets(const std::vector<FSys::path> &in_list) {
    image_loader l_image_load{};
    p_list.data = ranges::to_vector(
        in_list | ranges::views::transform([&](const FSys::path &in_path) {
          auto k_h = make_handle();
          k_h.emplace<assets_file>(in_path);
          k_h.emplace<assets>("null");
          if (use_time.data)
            this->add_time(k_h, in_path);
          if (use_icon.data)
            this->find_icon(k_h, in_path);
          k_h.emplace<database>();
          k_h.patch<database>(database::save);
          return k_h;
        }));

    DOODLE_LOG_INFO("检查到拖入文件:\n{}", fmt::join(in_list, "\n"));
    g_reg()->ctx<core_sig>().filter_handle(p_list.data);
  }

  gui_cache<std::vector<entt::handle>> p_list;

  gui_cache<bool> use_time;
  gui_cache<bool> use_icon;

 public:
  add_assets_for_file()
      : p_list("文件列表"s, std::vector<entt::handle>{}),
        use_time("检查时间"s, false),
        use_icon("寻找图标"s, true) {}

  void render(const entt::handle &) override {
    ImGui::Checkbox(*use_time.gui_name, &use_time.data);
    ImGui::Checkbox(*use_icon.gui_name, &use_icon.data);

    {
      dear::ListBox k_list{*p_list.gui_name};
      k_list &&[this]() {
        for (auto &&i : p_list.data) {
          if (i.all_of<assets_file>()) {
            dear::Text(i.get<assets_file>().p_name);
          }
        }
      };
    }

    dear::DragDropTarget{} && [&]() {
      if (auto *l_pay = ImGui::AcceptDragDropPayload(doodle_config::drop_imgui_id.data()); l_pay) {
        auto k_list = reinterpret_cast<drop_file_data *>(l_pay->Data);
        add_assets(k_list->files_);
      }
    };
  };
};

}  // namespace gui

class edit_widgets::impl {
 public:
  std::vector<boost::signals2::scoped_connection> p_sc;

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

  gui::database_edit data_edit;

  using gui_edit_cache = gui::gui_cache<std::unique_ptr<gui::edit_interface>>;
  using gui_add_cache  = gui::gui_cache<std::unique_ptr<gui::base_render>>;
  std::vector<gui_edit_cache> p_edit;
  std::vector<gui_add_cache> p_add;

  bool only_rand{false};
};

edit_widgets::edit_widgets()
    : p_i(std::make_unique<impl>()) {
  p_i->p_edit.emplace_back("季数编辑"s, std::make_unique<season_edit>());
  p_i->p_edit.emplace_back("集数编辑"s, std::make_unique<episodes_edit>());
  p_i->p_edit.emplace_back("镜头编辑"s, std::make_unique<shot_edit>());
  p_i->p_edit.emplace_back("文件编辑"s, std::make_unique<assets_file_edit>());
  p_i->p_edit.emplace_back("时间编辑"s, std::make_unique<time_edit>());
  boost::for_each(p_i->p_edit, [this](impl::gui_edit_cache &in_edit) {
    p_i->data_edit.link_sig(in_edit.data);
  });

  p_i->p_add.emplace_back("文件添加"s, std::make_unique<gui::add_assets_for_file>());
}
edit_widgets::~edit_widgets() = default;

void edit_widgets::init() {
  g_reg()->set<edit_widgets &>(*this);
  auto &l_sig = g_reg()->ctx<core_sig>();
  p_i->p_sc.emplace_back(l_sig.select_handle.connect(
      [&](const entt::handle &in) {
        p_i->p_h = in;
        p_i->data_edit.init(p_i->p_h);
        boost::for_each(p_i->p_edit, [&](impl::gui_edit_cache &in_edit) {
          in_edit.data->init(in);
        });
      }));
  p_i->p_sc.emplace_back(l_sig.project_begin_open.connect(
      [&](const std::filesystem::path &) {
        this->p_i->add_handles.clear();
        this->p_i->p_h       = {};
        this->p_i->only_rand = true;
      }));
  p_i->p_sc.emplace_back(
      l_sig.project_end_open.connect([&](const entt::handle &, const doodle::project &) {
        this->p_i->only_rand = false;
      }));
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
void edit_widgets::update(const chrono::duration<
                              chrono::system_clock::rep,
                              chrono::system_clock::period> &,
                          void *data) {
  dear::Disabled _l_rand{p_i->only_rand};

  dear::TreeNode{"添加"} && [this]() {
    this->add_handle();
  };
  dear::TreeNode{"编辑"} && [this]() {
    this->edit_handle();
  };
}

void edit_widgets::edit_handle() {
  /// @brief 资产编辑
  if (p_i->p_h) {
    p_i->data_edit.render(p_i->p_h);
    boost::for_each(p_i->p_edit, [&](impl::gui_edit_cache &in_edit) {
      dear::Text(in_edit.gui_name.name);
      in_edit.data->render(p_i->p_h);
      in_edit.data->save(p_i->p_h);
    });
    p_i->data_edit.save(p_i->p_h);
  }

  //  p_i->data_edit.save(p_i->p_h);
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

  for (auto &&l_add : p_i->p_add) {
    dear::Text(l_add.gui_name.name);
    l_add.data->render();
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
    std::vector<entt::handle> l_vector{};
    std::vector<entt::handle> k_list_h{};

    boost::copy(p_i->add_handles, std::back_inserter(k_list_h));

    boost::copy(
        boost::unique(boost::sort(k_list_h)) |
            boost::adaptors::filtered([](const entt::handle &in) -> bool {
              return in.valid();
            }),
        std::back_inserter(l_vector));
    g_reg()->ctx<core_sig>().filter_handle(l_vector);
  }
}
}  // namespace doodle

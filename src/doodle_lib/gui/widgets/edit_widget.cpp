
//
// Created by TD on 2021/9/23.
//

#include "edit_widget.h"

#include <doodle_core/core/core_sig.h>
#include <doodle_core/core/init_register.h>
#include <doodle_core/metadata/assets_file.h>
#include <doodle_core/metadata/image_icon.h>
#include <doodle_core/metadata/importance.h>
#include <doodle_core/metadata/metadata.h>
#include <doodle_core/metadata/metadata_cpp.h>

#include <doodle_app/gui/base/ref_base.h>

#include <doodle_lib/core/image_loader.h>
#include <doodle_lib/gui/widgets/assets_file_widgets.h>
#include <doodle_lib/gui/widgets/assets_filter_widget.h>
#include <doodle_lib/gui/widgets/edit_widgets/edit_user.h>
#include <doodle_lib/gui/widgets/edit_widgets/redirection_path_info_edit.h>

#include <boost/signals2/connection.hpp>

#include <core/tree_node.h>
#include <gui/widgets/database_edit.h>

namespace doodle::gui {
namespace edit_widgets_ns {}

class assets_edit : public edit_interface {
  using gui_list_item_type = gui_cache<std::string, gui_cache_path>;
  using gui_assets_list    = gui_cache<std::vector<gui_list_item_type>>;
  gui_assets_list path_list;

  edit_widgets_ns::edit_assets_data edit_data;

 public:
  assets_edit() : path_list("标签列表"s, std::vector<gui_list_item_type>{}) {
    // p_obs.connect(*g_reg(), entt::collector.update<data_type>());
  }

  void init(const std::vector<entt::handle> &in) override {
    edit_interface::init(in);
    path_list.data.clear();
    edit_data = {};

    auto l_r =
        in | ranges::views::filter([](const entt::handle &in_handle) -> bool {
          return in_handle && in_handle.any_of<assets>();
        }) |
        ranges::views::transform([](const entt::handle &in_handle) -> std::vector<std::string> {
          return in_handle.get<assets>().get_path_component();
        }) |
        ranges::to_vector |
        ranges::actions::sort([](const std::vector<std::string> &l_l, const std::vector<std::string> &l_r) -> bool {
          return l_l.size() < l_r.size();
        });
    if (l_r.empty()) return;
    auto l_list = l_r.front();
    FSys::path l_p_root{};
    ranges::for_each(l_list, [&](const std::string &in_string) {
      auto &l_p = path_list.data.emplace_back("##ass_edit"s, in_string);
      if (l_p_root.empty())
        l_p_root = in_string;
      else
        l_p_root /= in_string;

      l_p.path = l_p_root;
    });
  }
  void render(const entt::handle &in) override {
    dear::ListBox{*path_list.gui_name} && [&]() {
      ranges::for_each(path_list.data, [this](gui_list_item_type &in_list) {
        if (ImGui::InputText(*in_list.gui_name, &in_list.data)) {
          edit_data.old_path = in_list.path;
          edit_data.new_name = in_list.data;
          this->set_modify(true);
        }
      });
    };
  }

 protected:
  void init_(const entt::handle &in) override {}

  void save_(const entt::handle &in) const override {
    auto &l_ass = in.get<assets>();
    if (FSys::is_sub_path(l_ass.p_path, edit_data.old_path)) {
      std::string l_out{l_ass.p_path.generic_string()};
      auto new_path = edit_data.old_path;
      new_path.remove_filename() /= edit_data.new_name;
      boost::replace_all(l_out, edit_data.old_path.generic_string(), new_path.generic_string());

      l_ass.set_path(l_out);
      in.patch<assets>();
      // @todo 这里要写拖拽
    }
  }
};
/**
 * @brief 季数编辑
 *
 */
class season_edit : public gui::edit_interface {
 public:
  std::int32_t p_season{};

  void init_(const entt::handle &in) override {
    if (in.any_of<season>())
      p_season = in.get<season>().get_season();
    else
      p_season = 0;
  }
  void render(const entt::handle &in) override {
    if (imgui::InputInt("季数", &p_season, 1, 9999)) set_modify(true);
  }
  void save_(const entt::handle &in) const override { in.emplace_or_replace<season>(p_season); }
};
class episodes_edit : public gui::edit_interface {
 public:
  std::int32_t p_eps{};

  void init_(const entt::handle &in) override {
    if (in.all_of<episodes>())
      p_eps = boost::numeric_cast<std::int32_t>(in.get<episodes>().p_episodes);
    else
      p_eps = 0;
  }
  void render(const entt::handle &in) override {
    if (imgui::InputInt("集数", &p_eps, 1, 9999)) set_modify(true);
    ;
  }
  void save_(const entt::handle &in) const override { in.emplace_or_replace<episodes>(p_eps); }
};
class shot_edit : public gui::edit_interface {
 public:
  std::int32_t p_shot{};
  std::string p_shot_ab_str{};

  void init_(const entt::handle &in) override {
    if (in.all_of<shot>()) {
      auto &l_s     = in.get<shot>();
      p_shot        = boost::numeric_cast<std::int32_t>(l_s.get_shot());
      p_shot_ab_str = l_s.get_shot_ab();
    } else {
      p_shot        = 0;
      p_shot_ab_str = "None";
    }
  }
  void render(const entt::handle &in) override {
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
  void save_(const entt::handle &in) const override { in.emplace_or_replace<shot>(p_shot, p_shot_ab_str); }
};
class assets_file_edit : public gui::edit_interface {
  class fun_cache {
   public:
    bool select{};
    std::function<void(assets_file &l_ass)> patch;
  };

 public:
  gui::gui_cache<std::string, fun_cache> p_path_cache;
  gui::gui_cache<std::string, fun_cache> p_name_cache;
  gui::gui_cache<std::int32_t, fun_cache> p_version_cache;
  assets_file_edit() : p_path_cache("路径"s, ""s), p_name_cache("名称"s, ""s), p_version_cache("版本"s, 0) {
    p_path_cache.patch = [this](assets_file &l_ass) {
      l_ass.path_attr(p_path_cache.data);
      p_path_cache.select = false;
    };
    p_name_cache.patch = [this](assets_file &l_ass) {
      l_ass.name_attr(p_name_cache);
      p_name_cache.select = false;
    };
    p_version_cache.patch = [this](assets_file &l_ass) {
      l_ass.version_attr(p_version_cache);
      p_version_cache.select = false;
    };
  };

  void init_(const entt::handle &in) override {
    if (in.all_of<assets_file>()) {
      auto &l_ass     = in.get<assets_file>();
      p_path_cache    = l_ass.path_attr().generic_string();
      p_name_cache    = l_ass.name_attr();
      p_version_cache = l_ass.version_attr();
    } else {
      p_path_cache      = g_reg()->ctx().get<project>().p_path.generic_string();
      p_name_cache.data = {};
      p_version_cache   = {};
    }
  }
  void render(const entt::handle &in) override {
    if (ImGui::InputText(*p_path_cache.gui_name, &p_path_cache.data)) {
      set_modify(true);
      p_path_cache.select = true;
    }
    if (ImGui::InputText(*p_name_cache.gui_name, &p_name_cache.data)) {
      set_modify(true);
      p_name_cache.select = true;
    }
    if (ImGui::InputInt(*p_version_cache.gui_name, &p_version_cache.data)) {
      set_modify(true);
      p_version_cache.select = true;
    }
  }
  void save_(const entt::handle &in) const override {
    if (p_path_cache.select) {
      in.patch<assets_file>(p_path_cache.patch);
    }
    if (p_name_cache.select) {
      in.patch<assets_file>(p_name_cache.patch);
    }
    if (p_version_cache.select) {
      in.patch<assets_file>(p_version_cache.patch);
    }
  }
};

class time_edit : public gui::edit_interface {
 public:
  gui::gui_cache<std::array<std::int32_t, 3>> time_ymd;
  gui::gui_cache<std::array<std::int32_t, 3>> time_hms;

 public:
  time_edit()
      : time_ymd("年月日"s, std::array<std::int32_t, 3>{0, 0, 0}),
        time_hms("时分秒"s, std::array<std::int32_t, 3>{0, 0, 0}) {}
  void init_(const entt::handle &in) override {
    auto l_com       = in.get_or_emplace<time_point_wrap>().compose();
    time_ymd.data[0] = boost::pfr::get<0>(l_com);
    time_ymd.data[1] = boost::pfr::get<1>(l_com);
    time_ymd.data[2] = boost::pfr::get<2>(l_com);
    time_hms.data[0] = boost::pfr::get<3>(l_com);
    time_hms.data[1] = boost::pfr::get<4>(l_com);
    time_hms.data[2] = boost::pfr::get<5>(l_com);
  }

  void render(const entt::handle &in) override {
    if (ImGui::InputInt3(*time_ymd.gui_name, time_ymd.data.data())) set_modify(true);
    if (ImGui::InputInt3(*time_hms.gui_name, time_hms.data.data())) set_modify(true);
  }
  void save_(const entt::handle &in) const override {
    in.emplace_or_replace<time_point_wrap>(
        time_ymd.data[0], time_ymd.data[1], time_ymd.data[2], time_hms.data[0], time_hms.data[1], time_hms.data[2]
    );
  }
};

class importance_edit : public edit_interface {
  gui_cache<std::string> p_importance;

 public:
  importance_edit() : p_importance("等级"s, ""s) {}
  void init_(const entt::handle &in) override {
    if (in.any_of<importance>())
      p_importance.data = in.get<importance>().cutoff_p;
    else
      p_importance.data = {};
  }
  void render(const entt::handle &in) override {
    if (ImGui::InputText(*p_importance.gui_name, &p_importance.data)) {
      set_modify(true);
    }
  }
  void save_(const entt::handle &in) const override { in.emplace_or_replace<importance>(p_importance.data); }
};

class command_edit : public edit_interface {
  gui_cache<std::string> p_command;

 public:
  command_edit() : p_command("备注"s, ""s) {}
  void init_(const entt::handle &in) override {
    if (in.any_of<comment>())
      p_command.data = in.get<comment>().get_comment();
    else
      p_command.data = {};
  }
  void render(const entt::handle &in) override {
    if (ImGui::InputText(*p_command.gui_name, &p_command.data)) {
      set_modify(true);
    }
  }
  void save_(const entt::handle &in) const override { in.emplace_or_replace<comment>(p_command.data); }
};

class add_assets_for_file : public base_render {
  class combox_show_name {
   public:
    std::string show_name;
  };

  gui_cache<std::vector<entt::handle>> p_list;

  gui_cache<bool> use_time;
  gui_cache<bool> use_icon;
  gui_cache<std::vector<std::string>, combox_show_name> assets_list;
  boost::signals2::scoped_connection p_sig1{}, p_sig2{};

 public:
  add_assets_for_file()
      : p_list("文件列表"s, std::vector<entt::handle>{}),
        use_time("检查时间"s, true),
        use_icon("寻找图标"s, true),
        assets_list("分类"s, std::vector<std::string>{}) {
    auto &l_sig       = g_reg()->ctx().get<core_sig>();
    p_sig1            = l_sig.project_end_open.connect([this]() {
      auto &prj                   = g_reg()->ctx().get<project_config::base_config>();
      this->assets_list           = prj.assets_list;

      this->assets_list.show_name = this->assets_list.data.empty() ? "null"s : this->assets_list.data.front();
    });
    p_sig2            = l_sig.save_end.connect([this]() {
      auto &prj         = g_reg()->ctx().get<project_config::base_config>();
      this->assets_list = prj.assets_list;
      if (!ranges::any_of(this->assets_list.data, [this](const auto &in) -> bool {
            return this->assets_list.show_name == in;
          })) {
        this->assets_list.show_name = this->assets_list.data.empty() ? "null"s : this->assets_list.data.front();
      }
    });

    auto &prj         = g_reg()->ctx().get<project_config::base_config>();
    this->assets_list = prj.assets_list;
    if (!ranges::any_of(this->assets_list.data, [this](const auto &in) -> bool {
          return this->assets_list.show_name == in;
        })) {
      this->assets_list.show_name = this->assets_list.data.empty() ? "null"s : this->assets_list.data.front();
    }
  }

  bool render(const entt::handle &) override {
    bool result{false};

    ImGui::Checkbox(*use_time.gui_name, &use_time.data);
    ImGui::Checkbox(*use_icon.gui_name, &use_icon.data);

    dear::Combo{*assets_list.gui_name, assets_list.show_name.data()} && [this]() {
      for (auto &&i : assets_list.data)
        if (ImGui::Selectable(i.data())) assets_list.show_name = i;
    };

    {
      dear::ListBox k_list{*p_list.gui_name};
      k_list &&[this]() {
        for (auto &&i : p_list.data) {
          if (i) {
            if (i.all_of<assets_file>()) {
              dear::Text(i.get<assets_file>().name_attr());
            }
          }
        }
      };
    }
    return result;
  };
};

class add_entt_base : public base_render {
 private:
  gui_cache<std::int32_t> add_size;
  gui_cache<std::vector<entt::handle>> list_handle;

 public:
  add_entt_base() : add_size("添加个数", 1), list_handle("添加"s, std::vector<entt::handle>{}){};
  bool render(const entt::handle &in) override {
    bool result{false};
    ImGui::InputInt(*add_size.gui_name, &add_size.data);
    ImGui::SameLine();
    if (ImGui::Button(*list_handle.gui_name)) {
      for (std::int32_t i = 0; i < add_size; ++i) {
        auto l_h = list_handle.data.emplace_back(make_handle());
        l_h.emplace<database>();
      }
      g_reg()->ctx().get<core_sig>().filter_handle(list_handle);
    }
    return result;
  }
};

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
  std::vector<entt::handle> p_h;

  database_edit data_edit;
  assets_edit *assets_edit;

  using gui_edit_cache = gui_cache<std::unique_ptr<edit_interface>>;
  using gui_add_cache  = gui_cache<std::unique_ptr<base_render>>;
  std::vector<gui_edit_cache> p_edit;
  std::vector<gui_add_cache> p_add;
  std::string title_name_;
  bool open{true};
};

edit_widgets::edit_widgets() : p_i(std::make_unique<impl>()) {
  p_i->title_name_ = std::string{name};
  init();
}
edit_widgets::~edit_widgets() = default;

void edit_widgets::init() {
  p_i->p_edit.emplace_back("季数编辑"s, std::make_unique<season_edit>());
  p_i->p_edit.emplace_back("集数编辑"s, std::make_unique<episodes_edit>());
  p_i->p_edit.emplace_back("镜头编辑"s, std::make_unique<shot_edit>());
  p_i->p_edit.emplace_back("文件编辑"s, std::make_unique<assets_file_edit>());
  p_i->p_edit.emplace_back("用户编辑"s, std::make_unique<edit_user>());
  p_i->p_edit.emplace_back("备注"s, std::make_unique<command_edit>());
  p_i->p_edit.emplace_back("等级"s, std::make_unique<importance_edit>());
  auto *l_edit     = p_i->p_edit.emplace_back("资产类别"s, std::make_unique<assets_edit>()).data.get();

  p_i->assets_edit = dynamic_cast<assets_edit *>(l_edit);

  p_i->p_edit.emplace_back("时间编辑"s, std::make_unique<time_edit>());
  p_i->p_edit.emplace_back("替换规则"s, std::make_unique<redirection_path_info_edit>());

  /// \brief 连接信号
  ranges::for_each(p_i->p_edit, [this](impl::gui_edit_cache &in_edit) { p_i->data_edit.link_sig(in_edit.data); });

  p_i->p_add.emplace_back("添加"s, std::make_unique<add_entt_base>());
  p_i->p_add.emplace_back("文件添加"s, std::make_unique<add_assets_for_file>());

  g_reg()->ctx().emplace<edit_widgets &>(*this);
  auto &l_sig = g_reg()->ctx().get<core_sig>();
  p_i->p_sc.emplace_back(l_sig.select_handles.connect([&](const std::vector<entt::handle> &in) {
    p_i->p_h = in;
    p_i->data_edit.init(p_i->p_h);
    ranges::for_each(p_i->p_edit, [&](impl::gui_edit_cache &in_edit) { in_edit.data->init(p_i->p_h); });
  }));
  /**
   * @brief 保存时禁用编辑
   */
  p_i->p_sc.emplace_back(l_sig.project_begin_open.connect([&](const FSys::path &) {
    this->p_i->add_handles.clear();
    this->p_i->p_h = {};
  }));
}

bool edit_widgets::render() {
  dear::TreeNode{"添加"} && [this]() { this->add_handle(); };
  dear::TreeNode{"编辑"} && [this]() { this->edit_handle(); };
  return p_i->open;
}

void edit_widgets::edit_handle() {
  /// @brief 资产编辑
  if (!p_i->p_h.empty()) {
    const auto l_args = p_i->p_h.size();
    if (l_args > 1) dear::Text(fmt::format("同时编辑了 {}个", l_args));
    p_i->data_edit.render(p_i->p_h.front());
    ranges::for_each(p_i->p_edit, [&](impl::gui_edit_cache &in_edit) {
      dear::Text(in_edit.gui_name.name);
      in_edit.data->render(p_i->p_h.front());
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
  for (auto &&l_add : p_i->p_add) {
    dear::Text(l_add.gui_name.name);
    l_add.data->render();
  }
}

const std::string &edit_widgets::title() const { return p_i->title_name_; }

}  // namespace doodle::gui

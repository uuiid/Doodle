//
// Created by TD on 2021/9/16.
//

#include "assets_file_widgets.h"

#include <doodle_lib/gui/action/command.h>
#include <doodle_lib/gui/action/command_files.h>
#include <doodle_lib/gui/action/command_meta.h>
#include <doodle_lib/gui/factory/attribute_factory_interface.h>
#include <doodle_lib/lib_warp/imgui_warp.h>
#include <doodle_lib/metadata/metadata_cpp.h>

#include <entt/entt.hpp>

namespace doodle {
class assets_file_widgets::impl {
 public:
  assets_file_widgets* self;
  impl(assets_file_widgets* in)
      : self(in) {
  }
  std::vector<column_ptr> p_colum_list;
  entt::handle p_current_select;
  bool set_select(const entt::handle& in_select) {
    p_current_select = in_select;
    self->select_change(p_current_select);
  }
};

class assets_file_widgets::table_column {
 public:
  impl* p_table;
  table_column(impl* in_table) : p_table(in_table), p_name(), p_width(0){};
  string p_name;
  std::uint32_t p_width;
  void render(const entt::handle& in_ptr) {
    imgui::TableNextColumn();
    frame_render(in_ptr);
  }
  virtual void frame_render(const entt::handle& in_ptr) = 0;

  class column_id : public assets_file_widgets::table_column {
   public:
    column_id(impl* in_table)
        : assets_file_widgets::table_column(in_table) {
      p_name  = "id";
      p_width = 6;
    };
    void frame_render(const entt::handle& in_ptr) {
      if (dear::Selectable(in_ptr.get<database>().get_id_str(),
                           in_ptr == p_table->p_current_select,
                           ImGuiSelectableFlags_SpanAllColumns)) {
        p_table->set_select(in_ptr);
      }
    };
  };
  class column_version : public assets_file_widgets::table_column {
   public:
    column_version(impl* in_table)
        : assets_file_widgets::table_column(in_table) {
      p_name  = "版本";
      p_width = 6;
    };
    void frame_render(const entt::handle& in_ptr) {
      if (in_ptr.all_of<assets_file>())
        dear::Text(in_ptr.get<assets_file>().get_version_str());
    };
  };
  class column_comment : public assets_file_widgets::table_column {
   public:
    column_comment(impl* in_table)
        : assets_file_widgets::table_column(in_table) {
      p_name  = "评论";
      p_width = 6;
    };
    void frame_render(const entt::handle& in_ptr) {
      if (in_ptr.all_of<comment_vector>()) {
        auto& k_com = in_ptr.get<comment_vector>();
        dear::Text(k_com.get().empty() ? std::string{} : k_com.get().front().get_comment());
      } else {
        dear::Text(std::string{});
      }
    };
  };
  class column_path : public assets_file_widgets::table_column {
   public:
    column_path(impl* in_table)
        : assets_file_widgets::table_column(in_table) {
      p_name  = "路径";
      p_width = 6;
    };
    void frame_render(const entt::handle& in_ptr) {
      if (in_ptr.all_of<assets_path_vector>()) {
        auto& k_path = in_ptr.get<assets_path_vector>();
        string k_all_str{};
        string k_line_str{};

        if (!k_path.get().empty()) {
          k_line_str = k_path.get().front().get_server_path().generic_string();
          k_all_str  = fmt::format("{}", k_path);
        }
        dear::Text(k_line_str.c_str());
        if (!k_all_str.empty()) {
          imgui::SameLine();
          dear::HelpMarker{"(...)", k_all_str.c_str()};
        }
      }
    };
  };
  class column_time : public assets_file_widgets::table_column {
   public:
    column_time(impl* in_table)
        : assets_file_widgets::table_column(in_table) {
      p_name  = "时间";
      p_width = 6;
    };
    void frame_render(const entt::handle& in_ptr) {
      if (in_ptr.all_of<time_point_wrap>())
        dear::Text(in_ptr.get<time_point_wrap>().show_str());
    };
  };
  class column_user : public assets_file_widgets::table_column {
   public:
    column_user(impl* in_table)
        : assets_file_widgets::table_column(in_table) {
      p_name  = "制作人";
      p_width = 6;
    };
    void frame_render(const entt::handle& in_ptr) {
      if (in_ptr.all_of<assets_file>())
        dear::Text(in_ptr.get<assets_file>().get_user());
    };
  };
  class column_assets : public assets_file_widgets::table_column {
   public:
    column_assets(impl* in_table)
        : assets_file_widgets::table_column(in_table) {
      p_name  = "资产";
      p_width = 6;
    };
    void frame_render(const entt::handle& in_ptr){

    };
  };
  class column_season : public assets_file_widgets::table_column {
   public:
    column_season(impl* in_table)
        : assets_file_widgets::table_column(in_table) {
      p_name  = "季数";
      p_width = 6;
    };
    void frame_render(const entt::handle& in_ptr){

    };
  };
  class column_episodes : public assets_file_widgets::table_column {
   public:
    column_episodes(impl* in_table)
        : assets_file_widgets::table_column(in_table) {
      p_name  = "集数";
      p_width = 6;
    };
    void frame_render(const entt::handle& in_ptr){

    };
  };
  class column_shot : public assets_file_widgets::table_column {
   public:
    column_shot(impl* in_table)
        : assets_file_widgets::table_column(in_table) {
      p_name  = "镜头";
      p_width = 6;
    };
    void frame_render(const entt::handle& in_ptr){

    };
  };
};

namespace details {
void table_column::render(const entt::handle& in_ptr) {
  imgui::TableNextColumn();
  p_render(in_ptr);
}
}  // namespace details

assets_file_widgets::assets_file_widgets()
    : p_root(),
      p_current_select(),
      p_colum_list(),
      p_impl(std::make_unique<impl>()) {
  p_class_name = "文件列表";
  p_factory    = new_object<attr_assets_file>();
  p_impl->p_colum_list.emplace_back(new_object<table_column::column_id>(p_impl.get()));
  p_impl->p_colum_list.emplace_back(new_object<table_column::column_version>(p_impl.get()));
  p_impl->p_colum_list.emplace_back(new_object<table_column::column_comment>(p_impl.get()));
  p_impl->p_colum_list.emplace_back(new_object<table_column::column_path>(p_impl.get()));
  p_impl->p_colum_list.emplace_back(new_object<table_column::column_time>(p_impl.get()));
  p_impl->p_colum_list.emplace_back(new_object<table_column::column_user>(p_impl.get()));
  p_impl->p_colum_list.emplace_back(new_object<table_column::column_assets>(p_impl.get()));
  p_impl->p_colum_list.emplace_back(new_object<table_column::column_season>(p_impl.get()));
  p_impl->p_colum_list.emplace_back(new_object<table_column::column_episodes>(p_impl.get()));
  p_impl->p_colum_list.emplace_back(new_object<table_column::column_shot>(p_impl.get()));
}

void assets_file_widgets::frame_render() {
  static auto flags{ImGuiTableFlags_::ImGuiTableFlags_SizingFixedFit |
                    ImGuiTableFlags_::ImGuiTableFlags_Resizable |
                    ImGuiTableFlags_::ImGuiTableFlags_BordersOuter |
                    ImGuiTableFlags_::ImGuiTableFlags_BordersV |
                    ImGuiTableFlags_::ImGuiTableFlags_ContextMenuInBody |
                    ImGuiTableFlags_::ImGuiTableFlags_ScrollX |
                    ImGuiTableFlags_::ImGuiTableFlags_ScrollY};
  auto k_ = g_reg()->try_ctx<handle_list>();
  if (k_) {
    auto& k_list = *k_;
    dear::Table{"attribute_widgets",
                boost::numeric_cast<std::int32_t>(p_colum_list.size()),
                flags} &&
        [this, &k_list]() {
          /// 添加表头
          for (auto& i : p_colum_list) {
            if (i->p_width != 0)
              imgui::TableSetupColumn(i->p_name.c_str(), 0, imgui::GetFontSize() * i->p_width);
            else
              imgui::TableSetupColumn(i->p_name.c_str());
          }

          imgui::TableHeadersRow();
          list_data l_data{};

          for (auto& k_h : k_list) {
            if (k_h.all_of<assets_file>()) {
              imgui::TableNextRow();
              for (auto& l_i : p_colum_list)
                l_i->render(k_h);
            }
          }
        };
  }
}

void assets_file_widgets::set_metadata(const entt::entity& in_ptr) {
  p_root = make_handle(in_ptr);
}

}  // namespace doodle

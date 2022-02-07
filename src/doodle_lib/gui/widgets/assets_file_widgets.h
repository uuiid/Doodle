//
// Created by TD on 2021/9/16.
//

#pragma once
#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/gui/base_windwos.h>

#include <boost/signals2.hpp>
namespace doodle {

class assets_file_widgets;
namespace details {
class DOODLELIB_API table_column {
 protected:
  void set_select(const entt::handle& in_);
  assets_file_widgets* table;

  bool virtual frame_render(const entt::handle& in_ptr) = 0;

 public:
  table_column(assets_file_widgets* in) : table(in), p_name(), p_render(), p_width(0){};
  string p_name;
  std::uint32_t p_width;
  std::function<bool(const entt::handle&)> p_render;
  void render(const entt::handle& in_ptr);
};

class column_id : public table_column {
 public:
  column_id(assets_file_widgets* in_table)
      : table_column(in_table) {
    p_name  = "id";
    p_width = 6;
  };
  bool frame_render(const entt::handle& in_ptr);
};
class column_version : public table_column {
 public:
  column_version(assets_file_widgets* in_table)
      : table_column(in_table) {
    p_name  = "版本";
    p_width = 6;
  };
  bool frame_render(const entt::handle& in_ptr);
};
class column_comment : public table_column {
 public:
  column_comment(assets_file_widgets* in_table)
      : table_column(in_table) {
    p_name  = "评论";
    p_width = 6;
  };
  bool frame_render(const entt::handle& in_ptr);
};
class column_path : public table_column {
 public:
  column_path(assets_file_widgets* in_table)
      : table_column(in_table) {
    p_name  = "路径";
    p_width = 6;
  };
  bool frame_render(const entt::handle& in_ptr);
};
class column_time : public table_column {
 public:
  column_time(assets_file_widgets* in_table)
      : table_column(in_table) {
    p_name  = "时间";
    p_width = 6;
  };
  bool frame_render(const entt::handle& in_ptr);
};
class column_user : public table_column {
 public:
  column_user(assets_file_widgets* in_table)
      : table_column(in_table) {
    p_name  = "制作人";
    p_width = 6;
  };
  bool frame_render(const entt::handle& in_ptr);
};
class column_assets : public table_column {
 public:
  column_assets(assets_file_widgets* in_table)
      : table_column(in_table) {
    p_name  = "资产";
    p_width = 6;
  };
  bool frame_render(const entt::handle& in_ptr);
};
class column_season : public table_column {
 public:
  column_season(assets_file_widgets* in_table)
      : table_column(in_table) {
    p_name  = "季数";
    p_width = 6;
  };
  bool frame_render(const entt::handle& in_ptr);
};
class column_episodes : public table_column {
 public:
  column_episodes(assets_file_widgets* in_table)
      : table_column(in_table) {
    p_name  = "集数";
    p_width = 6;
  };
  bool frame_render(const entt::handle& in_ptr);
};
class column_shot : public table_column {
 public:
  column_shot(assets_file_widgets* in_table)
      : table_column(in_table) {
    p_name  = "镜头";
    p_width = 6;
  };
  bool frame_render(const entt::handle& in_ptr);
};

using table_column_ptr = std::shared_ptr<table_column>;

class icon_render {
 public:
  void render(const entt::handle& in);
};

}  // namespace details

/**
 * @brief 文件列表显示
 * @image html assets_file_widgets.jpg  显示的文件
 * 文件列表显示了在资产项目下所有的文件文件
 * 在显示的文件中，路径并不是所有的， 而是最主要的一条
 * @note 每次上传文件都会递增版本号， 如果需要新的条目请创建新条目
 *
 */
class DOODLELIB_API assets_file_widgets : public process_t<assets_file_widgets> {
  friend details::table_column_ptr;

 

  void set_select(const entt::handle& in_);
  void render_context_menu(const entt::handle& in_);

 public:
  entt::entity p_current_select;

  assets_file_widgets();
  ~assets_file_widgets() override;

  constexpr static std::string_view name{"文件列表"};

  [[maybe_unused]] void init();
  [[maybe_unused]] void succeeded();
  [[maybe_unused]] void failed();
  [[maybe_unused]] void aborted();
  [[maybe_unused]] void update(delta_type, void* data);

  boost::signals2::signal<void(const entt::entity&)> select_change;
};

}  // namespace doodle

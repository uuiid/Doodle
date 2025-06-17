//
// Created by TD on 25-5-15.
//

#include <doodle_core/metadata/assets_file.h>
#include <doodle_core/sqlite_orm/detail/sqlite_database_impl.h>
#include <doodle_core/sqlite_orm/sqlite_database.h>
#include <doodle_core/sqlite_orm/sqlite_upgrade.h>

#include <sqlite_orm/sqlite_orm.h>
namespace doodle::details {

struct upgrade_init_t : sqlite_upgrade {
  // 0196eb9d5dc0727d8a751b05dea8494d
  static constexpr uuid g_open_id{0x75, 0x5c, 0x9e, 0xdd, 0x94, 0x81, 0x41, 0x45,
                                  0xab, 0x43, 0x21, 0x49, 0x1b, 0xdf, 0x27, 0x39};
  // 755c9edd-9481-4145-ab43-21491bdf2739
  static constexpr uuid g_lable_id{0x01, 0x96, 0xeb, 0x9d, 0x5d, 0xc0, 0x72, 0x7d,
                                   0x8a, 0x75, 0x1b, 0x05, 0xde, 0xa8, 0x49, 0x4d};
  // 5159f210-7ec8-40e3-b8c9-2a06d0b4b116
  static constexpr uuid g_closed_id{0x51, 0x59, 0xf2, 0x10, 0x7e, 0xc8, 0x40, 0xe3,
                                    0xb8, 0xc9, 0x2a, 0x06, 0xd0, 0xb4, 0xb1, 0x16};
  explicit upgrade_init_t(const FSys::path& in_path) {}
  void upgrade(const std::shared_ptr<sqlite_database_impl>& in_data) override {
    if (in_data->uuid_to_id<assets_helper::database_t>(g_lable_id) == 0) {
      auto l_label      = std::make_shared<assets_helper::database_t>();
      l_label->uuid_id_ = g_lable_id;
      l_label->label_   = "标签";
      in_data->install_unsafe<assets_helper::database_t>(l_label);
    }
    if (in_data->uuid_to_id<project_status>(g_open_id) == 0) {
      auto l_s    = std::make_shared<project_status>();
      l_s->name_  = "Open";
      l_s->color_ = "#000000";
      in_data->install_unsafe<project_status>(l_s);
    }
    if (in_data->uuid_to_id<project_status>(g_closed_id) == 0) {
      auto l_s    = std::make_shared<project_status>();
      l_s->name_  = "Closed";
      l_s->color_ = "#000000";
      in_data->install_unsafe<project_status>(l_s);
    }
  }
};

struct upgrade_1_t : sqlite_upgrade {
  struct impl_data {
    std::int32_t id_{};
    uuid uuid_id_{};
    std::string label_{};
    FSys::path path_{};
    std::string notes_{};
    // 激活
    bool active_{};
    uuid uuid_parent_{};
    std::int32_t parent_id_{};
    bool has_thumbnail_{};
    std::string extension_{};
    operator assets_file_helper::database_t() const {
      assets_file_helper::database_t l_ret{};
      l_ret.uuid_id_ = uuid_id_;
      l_ret.label_   = label_;
      l_ret.path_    = path_;
      l_ret.notes_   = notes_;
      l_ret.active_  = active_;
      l_ret.uuid_parents_.emplace_back(uuid_parent_);
      l_ret.has_thumbnail_ = has_thumbnail_;
      l_ret.extension_     = extension_;
      return l_ret;
    }
  };
  std::shared_ptr<std::vector<assets_file_helper::database_t>> assets_file_list_{};
  std::shared_ptr<std::vector<assets_file_helper::link_parent_t>> link_parent_list_{};
  explicit upgrade_1_t(const FSys::path& in_path) {
    using namespace sqlite_orm;
    auto l_p = make_storage(
        in_path.generic_string(), connection_control{},
        make_table(
            "assets_file_tab",  //
            make_column("id", &impl_data::id_, primary_key()),
            make_column("uuid_id", &impl_data::uuid_id_, unique(), not_null()),
            make_column("label", &impl_data::label_), make_column("parent_uuid", &impl_data::uuid_parent_),
            make_column("path", &impl_data::path_), make_column("notes", &impl_data::notes_),
            make_column("active", &impl_data::active_), make_column("parent_id", &impl_data::parent_id_),
            make_column("has_thumbnail", &impl_data::has_thumbnail_, default_value(false)),
            make_column("extension", &impl_data::extension_, default_value(".png"s))
        )
    );
    l_p.open_forever();
    if (!l_p.table_exists("assets_file_tab")) return;
    assets_file_list_ = std::make_shared<std::vector<assets_file_helper::database_t>>();
    link_parent_list_ = std::make_shared<std::vector<assets_file_helper::link_parent_t>>();
    for (auto&& i : l_p.get_all<impl_data>()) {
      assets_file_list_->emplace_back(i);
      if (!i.uuid_parent_.is_nil())
        link_parent_list_->emplace_back(
            assets_file_helper::link_parent_t{.assets_type_uuid_ = i.uuid_parent_, .assets_uuid_ = i.uuid_id_}
        );
    }
    l_p.drop_table_if_exists("assets_file_tab");
  }
  void upgrade(const std::shared_ptr<sqlite_database_impl>& in_data) override {
    if (assets_file_list_ && link_parent_list_) {
      in_data->install_range_unsafe(assets_file_list_);
      in_data->install_range_unsafe(link_parent_list_);
    }
  }
  ~upgrade_1_t() override = default;
};
std::shared_ptr<sqlite_upgrade> upgrade_init(const FSys::path& in_db_path) {
  return std::make_shared<upgrade_init_t>(in_db_path);
}
std::shared_ptr<sqlite_upgrade> upgrade_1(const FSys::path& in_db_path) {
  return std::make_shared<upgrade_1_t>(in_db_path);
}

}  // namespace doodle::details
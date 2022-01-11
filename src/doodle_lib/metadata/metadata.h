//
// Created by teXiao on 2021/4/27.
//

#pragma once

#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/core/observable_container.h>
#include <doodle_lib/metadata/metadata_factory.h>

#include <boost/signals2.hpp>
#include <optional>

namespace doodle {
class DOODLELIB_API metadata_database {
 public:
  std::string user_data;
  std::uint64_t id;
  std::optional<std::int32_t> parent;
  std::int32_t m_type;
  std::string uuid_path;
  std::optional<std::int32_t> season;
  std::optional<std::int32_t> episode;
  std::optional<std::int32_t> shot;
  std::optional<std::string> assets;
};
enum class metadata_type {
  unknown_file       = 0,
  project_root       = 1,
  file               = 2,
  folder             = 3,
  derive_file        = 4,
  animation_lib_root = 5,
  maya_file          = 6,
  maya_rig           = 7,
  fbx                = 8,
  abc                = 9,
  movie              = 10,
  ue4_prj            = 11

};

class DOODLELIB_API root_ref {
 public:
  entt::entity p_root;

  inline entt::handle root_handle() {
    return make_handle(p_root);
  }

  inline void set_root(entt::entity in_root) {
    p_root = in_root;
  }
};

class DOODLELIB_API database_root {
 public:
  std::uint64_t p_current_id;
  std::uint64_t p_cout_rows;

  bool p_end;
  database_root()
      : p_current_id(0),
        p_cout_rows(0),
        p_end(false) {}

  [[nodiscard]] bool is_end() const;
  [[nodiscard]] const std::uint64_t &get_current_id() const;
  void reset();
};

class DOODLELIB_API database {
  friend rpc_metadata_client;
  friend metadata_serialize;

 private:
  mutable std::uint64_t p_id;
  mutable string p_id_str;
  std::optional<uint64_t> p_parent_id;
  metadata_type p_type;
  FSys::path p_uuid;
  std::uint32_t p_boost_serialize_vesion;
  boost::uuids::uuid p_uuid_;
  void set_id(std::uint64_t in_id) const;

 public:
  database();
  ~database();

  bool has_components() const;

  DOODLE_IMP_MOVE(database);
  static void set_enum(entt::registry &in_reg, entt::entity in_ent);

  const FSys::path &get_url_uuid() const;
  std::int32_t get_meta_type_int() const;
  bool is_install() const;
  const string &get_id_str() const;

  const boost::uuids::uuid &uuid() const;
  /**
   * @brief 获得数据库id
   *
   * @return std::int32_t
   */
  std::uint64_t get_id() const;

  /**
   * @brief 设置数据库中的类型
   *
   * @param in_meta 类型
   */
  void set_meta_type(const metadata_type &in_meta);
  void set_meta_type(const std::string &in_meta);
  void set_meta_type(std::int32_t in_);

  database &operator=(const metadata_database &in);
  explicit operator metadata_database() const;
  bool operator==(const database &in_rhs) const;
  bool operator==(const boost::uuids::uuid &in_rhs) const;
  bool operator!=(const database &in_rhs) const;
  bool operator!=(const boost::uuids::uuid &in_rhs) const;

  friend void to_json(nlohmann::json &j, const database &p) {
    j["id"]        = p.p_id;
    j["parent_id"] = p.p_parent_id;
    j["type"]      = p.p_type;
    j["uuid_"]     = p.p_uuid_;
  }
  friend void from_json(const nlohmann::json &j, database &p) {
    j.at("id").get_to(p.p_id);
    j.at("parent_id").get_to(p.p_parent_id);
    j.at("type").get_to(p.p_type);
    j.at("uuid_").get_to(p.p_uuid_);
    p.p_uuid = boost::uuids::to_string(p.p_uuid_);
  }
};

// using to_str = entt::tag<"to_str"_hs>;
using need_load      = entt::tag<"need_load"_hs>;
using need_root_load = entt::tag<"need_root_load"_hs>;
using is_load        = entt::tag<"is_loaded"_hs>;
using need_save      = entt::tag<"need_save"_hs>;
using need_delete    = entt::tag<"need_delete"_hs>;

class DOODLELIB_API database_stauts {
  std::int32_t p_statu;

 public:
  template <class in_statu>
  void set() {
    p_statu = in_statu::value;
  };
  template <class in_statu>
  [[nodiscard]] bool is() const {
    return p_statu == in_statu::value;
  }

  template <class in_class>
  class DOODLELIB_API set_status {
   public:
    set_status() = default;
    template <class in_comm>
    void operator()(in_comm &in) {
      in.template set<in_class>();
    };
  };
};

template <class in_class>
using database_set_stauts = database_stauts::set_status<in_class>;

class DOODLELIB_API to_str {
 private:
  mutable std::string p_str;

 public:
  to_str() = default;
  DOODLE_MOVE(to_str);

  const string &get() const;
  operator string() const;
};

template <class in_class>
class DOODLELIB_API handle_warp {
 public:
  entt::handle handle_;
};

}  // namespace doodle

// CEREAL_REGISTER_TYPE(doodle::metadata)
// CEREAL_REGISTER_POLYMORPHIC_RELATION(std::enable_shared_from_this<doodle::metadata>, doodle::metadata)

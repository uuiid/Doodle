//
// Created by teXiao on 2021/4/27.
//

#pragma once
#include <doodle_lib/core/observable_container.h>
#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/lib_warp/protobuf_warp.h>
#include <doodle_lib/metadata/metadata_factory.h>
#include <doodle_lib/metadata/tree_adapter.h>

#include <any>
#include <boost/intrusive/intrusive_fwd.hpp>
#include <boost/intrusive/link_mode.hpp>
#include <boost/intrusive/list.hpp>
#include <boost/intrusive/pack_options.hpp>
#include <boost/intrusive/set.hpp>
#include <boost/intrusive/trivial_value_traits.hpp>
#include <boost/serialization/export.hpp>
#include <boost/signals2.hpp>
#include <optional>

namespace doodle {

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

class DOODLELIB_API tree_relationship
/* : public boost::intrusive::set_base_hook<> */ {
  friend metadata_serialize;

 private:
  entt::entity p_parent;

  void set_parent_raw(const entt::handle &in_parent);

 public:
  std::vector<entt::entity> p_child;
  // boost::intrusive::set<> p_child;

  DOODLE_MOVE(tree_relationship)

  tree_relationship::tree_relationship()
      : p_parent(entt::null),
        p_child() {
  }
  tree_relationship(entt::entity in_parent)
      : tree_relationship() {
    set_parent(std::move(in_parent));
  }
  bool has_parent() const;

  template <class parent_class>
  parent_class *find_parent_class() {
    entt::handle k_h = find_parent_class_h<parent_class>();
    if (k_h) {
      return k_h.try_get<parent_class>();
    }
    return nullptr;
  }

  template <class parent_class>
  entt::handle find_parent_class_h() {
    auto k_p = make_handle(p_parent);
    while (k_p) {
      if (k_p.any_of<parent_class>())
        return k_p;
      k_p = k_p.get<tree_relationship>().get_parent_h();
    }
    return {};
  }

  [[nodiscard]] entt::entity get_parent() const noexcept;
  [[nodiscard]] entt::handle get_parent_h() const noexcept;
  void set_parent(const entt::entity &in_parent) noexcept;

  [[nodiscard]] const std::vector<entt::entity> &get_child() const noexcept;
  // [[nodiscard]] std::vector<entt::entity> &get_child() noexcept;
  // void set_child(const std::vector<entt::entity> &in_child) noexcept;

  entt::handle get_root() const;
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
  friend tree_relationship;
  friend metadata_serialize;

 private:
  mutable std::uint64_t p_id;
  mutable string p_id_str;
  std::optional<uint64_t> p_parent_id;
  metadata_type p_type;
  std::string p_uuid;
  std::uint32_t p_boost_serialize_vesion;
  boost::uuids::uuid p_uuid_;
  void set_id(std::uint64_t in_id) const;

 public:
  database();
  ~database();

  bool has_components() const;

  DOODLE_IMP_MOVE(database);
  static void set_enum(entt::registry &in_reg, entt::entity in_ent);

  FSys::path get_url_uuid() const;
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
  bool operator!=(const database &in_rhs) const;

  friend class boost::serialization::access;
  template <class Archive>
  void save(Archive &ar, const std::uint32_t version) const {
    ar &BOOST_SERIALIZATION_NVP(p_id);
    ar &BOOST_SERIALIZATION_NVP(p_parent_id);
    ar &BOOST_SERIALIZATION_NVP(p_type);
    ar &BOOST_SERIALIZATION_NVP(p_uuid_);
  };

  template <class Archive>
  void load(Archive &ar, const std::uint32_t version) {
    p_boost_serialize_vesion = version;
    if (version == 1) {
      ar &BOOST_SERIALIZATION_NVP(p_id);
      ar &BOOST_SERIALIZATION_NVP(p_parent_id);
      ar &BOOST_SERIALIZATION_NVP(p_type);
      ar &BOOST_SERIALIZATION_NVP(p_uuid);
      std::size_t k_1;
      std::size_t k_2;
      ar &BOOST_SERIALIZATION_NVP(k_1);
      ar &BOOST_SERIALIZATION_NVP(k_2);
      p_uuid_ = boost::lexical_cast<boost::uuids::uuid>(p_uuid);
    }
    if (version == 2) {
      ar &BOOST_SERIALIZATION_NVP(p_id);
      ar &BOOST_SERIALIZATION_NVP(p_parent_id);
      ar &BOOST_SERIALIZATION_NVP(p_type);
      ar &BOOST_SERIALIZATION_NVP(p_uuid_);
      p_uuid = boost::uuids::to_string(p_uuid_);
    }
  };

  BOOST_SERIALIZATION_SPLIT_MEMBER()
};

// using to_str = entt::tag<"to_str"_hs>;
using need_load      = entt::tag<"need_load"_hs>;
using need_root_load = entt::tag<"need_root_load"_hs>;
using is_load        = entt::tag<"is_load"_hs>;
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
  bool is() const {
    return p_statu == in_statu::value;
  }

  template <class in_class>
  class DOODLELIB_API set_status {
   public:
    set_status() = default;
    template <class in_comm>
    void operator()(in_comm &in) {
      in.set<in_class>();
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

BOOST_CLASS_VERSION(doodle::database, 2)
BOOST_SERIALIZATION_ASSUME_ABSTRACT(doodle::database)
BOOST_CLASS_EXPORT_KEY(doodle::database)

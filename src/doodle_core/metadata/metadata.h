//
// Created by teXiao on 2021/4/27.
//

#pragma once

#include <doodle_core/doodle_core_fwd.h>

#include <boost/signals2.hpp>
#include <optional>

namespace doodle {
class DOODLE_CORE_EXPORT metadata_database {
 public:
  std::string user_data;
  std::uint64_t id;
  std::optional<std::uint32_t> parent;
  std::int32_t m_type;
  std::string uuid_path;
  boost::uuids::uuid uuid_;

  std::optional<std::int32_t> season;
  std::optional<std::int32_t> episode;
  std::optional<std::int32_t> shot;
  std::optional<std::string> assets;
};
/**
 * @brief 元数据代表的类型
 *
 */
enum class metadata_type : std::int32_t {
  unknown_file       = 0,
  project_root       = 1,  /// 项目的根
  file               = 2,  /// 文件, 基本可以说具有 assets_file 类组件
  folder             = 3,  /// 没有文件,没有 assets_file 组件
  derive_file        = 4,
  animation_lib_root = 5,
  maya_file          = 6,
  maya_rig           = 7,
  fbx                = 8,
  abc                = 9,
  movie              = 10,
  ue4_prj            = 11

};

class DOODLE_CORE_EXPORT database_info {
 public:
  FSys::path path_;
};

using data_status_save   = entt::tag<"data_status_save"_hs>;
using data_status_delete = entt::tag<"data_status_delete"_hs>;

class DOODLE_CORE_EXPORT database {
 private:
  friend class database_task_install;

  friend class database_n::insert;
  friend class database_n::select;
  friend class database_n::update_data;
  friend class database_n::delete_data;
  class impl;
  std::unique_ptr<impl> p_i;
  void set_id(std::uint64_t in_id) const;

 public:
  class DOODLE_CORE_EXPORT ref_data {
    friend void to_json(nlohmann::json &j, const ref_data &p);
    friend void from_json(const nlohmann::json &j, ref_data &p);

    bool find_for_path(const FSys::path &in_path);

   public:
    ref_data();
    explicit ref_data(const database &in);
    std::uint64_t id;
    boost::uuids::uuid uuid;

    operator bool() const;
    entt::handle handle() const;

    bool operator==(const ref_data &in_rhs) const;
    bool operator!=(const ref_data &in_rhs) const;
  };

  database();
  explicit database(const std::string &in_uuid_str);
  //  explicit database(const metadata_database &in_metadata_database);
  ~database();

  bool has_components() const;

  database(database &&) noexcept;
  database &operator=(database &&) noexcept;

  database(database &) noexcept            = delete;
  database &operator=(database &) noexcept = delete;

  static void set_enum(entt::registry &in_reg, entt::entity in_ent);

  bool is_install() const;

  const boost::uuids::uuid &uuid() const;
  /**
   * @brief 获得数据库id
   *
   * @return std::int32_t
   */
  std::uint64_t get_id() const;

  bool operator==(const database &in_rhs) const;
  bool operator!=(const database &in_rhs) const;

  bool operator==(const ref_data &in_rhs) const;
  bool operator!=(const ref_data &in_rhs) const;
  bool operator==(const std::string &in_uuid_str) const;
  bool operator!=(const std::string &in_uuid_str) const;

  bool operator==(const boost::uuids::uuid &in_rhs) const;
  bool operator!=(const boost::uuids::uuid &in_rhs) const;

  enum class status : std::uint8_t {
    none        = 0,
    is_sync     = 1,
    need_load   = 2,
    need_save   = 3,
    need_delete = 4,
  };

  std::atomic<status> status_;

  friend void to_json(nlohmann::json &j, const database &p);
  friend void from_json(const nlohmann::json &j, database &p);

  class DOODLE_CORE_EXPORT fun_save_ {
   public:
    constexpr fun_save_() = default;
    void operator()(const entt::handle &in) const {
      in.get_or_emplace<data_status_save>();
    }
  };
  class DOODLE_CORE_EXPORT fun_delete_ {
   public:
    constexpr fun_delete_() = default;
    void operator()(const entt::handle &in) const {
      in.get_or_emplace<data_status_delete>();
    }
  };

  constexpr const static fun_save_ save{};
  constexpr const static fun_delete_ delete_{};
};

// using need_save = entt::tag<"need_save"_hs>;

template <class in_class>
class DOODLE_CORE_EXPORT handle_warp {
 public:
  entt::handle handle_;
};

}  // namespace doodle

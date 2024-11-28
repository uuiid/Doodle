//
// Created by teXiao on 2021/4/27.
//

#pragma once

#include "doodle_core/core/core_help_impl.h"
#include <doodle_core/doodle_core_fwd.h>

#include "boost/lexical_cast.hpp"
#include "boost/uuid/uuid.hpp"

#include <entt/core/type_info.hpp>
#include <optional>

namespace doodle {
class database;
namespace snapshot {
void load_com(database &in_entity, std::shared_ptr<void> &in_pre);
}

void DOODLE_CORE_API to_json(nlohmann::json &j, const database &p);
void DOODLE_CORE_API from_json(const nlohmann::json &j, database &p);

/**
 * 这个类可以按照id排序
 */
class DOODLE_CORE_API database : boost::totally_ordered<database>, boost::equality_comparable<boost::uuids::uuid> {
  friend void ::doodle::snapshot::load_com(database &in_entity, std::shared_ptr<void> &in_pre);

 private:
  mutable std::uint64_t p_id;
  boost::uuids::uuid p_uuid_;

  void set_id(std::uint64_t in_id) const;

 public:
  database();
  explicit database(const std::string &in_uuid_str);
  explicit database(const boost::uuids::uuid &in);

  virtual ~database();

  database(database &&) noexcept                 = default;
  database &operator=(database &&) noexcept      = default;

  database(const database &) noexcept            = default;
  database &operator=(const database &) noexcept = default;

  bool is_install() const;

  const boost::uuids::uuid &uuid() const;
  /**
   * @brief 获得数据库id
   *
   * @return std::int32_t
   */
  std::uint64_t get_id() const;

  bool operator==(const database &in_rhs) const;
  bool operator<(const database &in_rhs) const;
  bool operator==(const boost::uuids::uuid &in_rhs) const;

  friend void DOODLE_CORE_API to_json(nlohmann::json &j, const database &p);
  friend void DOODLE_CORE_API from_json(const nlohmann::json &j, database &p);

  static entt::handle find_by_uuid(const boost::uuids::uuid &in);
  inline static entt::handle find_by_uuid(const std::string &in) {
    return find_by_uuid(boost::lexical_cast<boost::uuids::uuid>(in));
  };
  [[nodiscard]] entt::handle find_by_uuid() const;
};

}  // namespace doodle

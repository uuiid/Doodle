//
// Created by teXiao on 2021/4/27.
//

#pragma once

#include "doodle_core/core/core_help_impl.h"
#include <doodle_core/core/core_sql.h>
#include <doodle_core/doodle_core_fwd.h>

#include <entt/core/type_info.hpp>
#include <optional>
#include <rttr/rttr_enable.h>

namespace doodle {

using data_status_save   = entt::tag<"data_status_save"_hs>;
using data_status_delete = entt::tag<"data_status_delete"_hs>;

class database;

namespace database_ns {
class DOODLE_CORE_API ref_data {
  RTTR_ENABLE();
  friend void to_json(nlohmann::json &j, const ref_data &p);
  friend void from_json(const nlohmann::json &j, ref_data &p);

 public:
  ref_data();
  explicit ref_data(const database &in);

  boost::uuids::uuid uuid{};

  explicit operator bool() const;
  /**
   * @brief 遍历所有的 带有数据库组件的实体
   *
   * @return entt::handle
   */
  [[nodiscard("")]] entt::handle handle() const;
  /**
   * @brief 遍历所有的 带有数据库组件的实体, 并使用模板类进行二次筛选加速迭代
   *
   * @tparam Args
   * @return entt::handle
   */
  template <typename... Args>
  [[nodiscard("")]] entt::handle handle() const;

  bool operator==(const ref_data &in_rhs) const;
};
}  // namespace database_ns

void DOODLE_CORE_API to_json(nlohmann::json &j, const database &p);
void DOODLE_CORE_API from_json(const nlohmann::json &j, database &p);

class DOODLE_CORE_API database : boost::equality_comparable<database>,
                                 boost::equality_comparable<boost::uuids::uuid>,
                                 boost::equality_comparable<database_ns::ref_data> {
  RTTR_ENABLE();
  template <typename T, bool constraint>
  friend struct database_n::sql_com;

 private:
  friend class database_n::insert;
  friend class database_n::select;
  friend class database_n::update_data;
  friend class database_n::delete_data;

  mutable std::uint64_t p_id;
  boost::uuids::uuid p_uuid_;

  void set_id(std::uint64_t in_id) const;

 public:
  using ref_data = database_ns::ref_data;

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
  bool operator==(const boost::uuids::uuid &in_rhs) const;
  bool operator==(const ref_data &in_rhs) const;

  friend void DOODLE_CORE_API to_json(nlohmann::json &j, const database &p);
  friend void DOODLE_CORE_API from_json(const nlohmann::json &j, database &p);

  static entt::handle find_by_uuid(const boost::uuids::uuid &in);
  [[nodiscard]] entt::handle find_by_uuid() const;

  class DOODLE_CORE_API fun_save_ {
   public:
    constexpr fun_save_() = default;
    void operator()(const entt::handle &in) const;
  };
  class DOODLE_CORE_API fun_delete_ {
   public:
    constexpr fun_delete_() = default;
    void operator()(const entt::handle &in) const;
  };

  constexpr const static fun_save_ save{};
  constexpr const static fun_delete_ delete_{};
};

namespace database_ns {
template <typename... Args>
entt::handle ref_data::handle() const {
  entt::handle l_r{};
  //  ranges::make_subrange(g_reg()->view<database>().each());
  if (!uuid.is_nil())
    for (auto &&e : g_reg()->view<database, Args...>()) {
      if (g_reg()->get<database>(e) == uuid) {
        l_r = entt::handle{*g_reg(), e};
        break;
      }
    }
  return l_r;
}

}  // namespace database_ns

}  // namespace doodle

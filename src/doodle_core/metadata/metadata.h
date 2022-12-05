//
// Created by teXiao on 2021/4/27.
//

#pragma once

#include <doodle_core/doodle_core_fwd.h>

#include <boost/signals2.hpp>

#include <optional>
#include <rttr/rttr_enable.h>

namespace doodle {

class DOODLE_CORE_API database_info {
  RTTR_ENABLE();

 public:
  FSys::path path_;
};

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
  [[nodiscard("")]] entt::handle handle() const;

  bool operator==(const ref_data &in_rhs) const;
};
}  // namespace database_ns

void DOODLE_CORE_API to_json(nlohmann::json &j, const database &p);
void DOODLE_CORE_API from_json(const nlohmann::json &j, database &p);


class DOODLE_CORE_API database
    : boost::equality_comparable<database>,
      boost::equality_comparable<boost::uuids::uuid>,
      boost::equality_comparable<database_ns::ref_data> {
  RTTR_ENABLE();

 private:
  friend class database_n::insert;
  friend class database_n::select;
  friend class database_n::update_data;
  friend class database_n::delete_data;
  class impl;
  std::unique_ptr<impl> p_i;
  void set_id(std::uint64_t in_id) const;

 public:
  using ref_data = database_ns::ref_data;

  database();
  explicit database(const std::string &in_uuid_str);
  explicit database(const boost::uuids::uuid &in);

  virtual ~database();

  database(database &&) noexcept;
  database &operator=(database &&) noexcept;

  database(const database &) noexcept;
  database &operator=(const database &) noexcept;

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

}  // namespace doodle

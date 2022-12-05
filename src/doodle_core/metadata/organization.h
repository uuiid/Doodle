#pragma once

#include <doodle_core/doodle_core_fwd.h>

#include <rttr/rttr_enable.h>

namespace doodle {
namespace project_config {
class base_config;
}

class organization {
 private:
  friend void to_json(nlohmann::json &j, const organization &p);
  friend void from_json(const nlohmann::json &j, organization &p);

  std::unique_ptr<project_config::base_config> p_config;
  RTTR_ENABLE();

 public:
  std::string org_p;
  organization();
  organization(std::string in_org_p);
  virtual ~organization();

  /**
   * @brief 构造一个新的id
   * @warning 这里需要转移 name 字符串视图指向的缓冲区
   *
   * @param in_r 传入的 id
   */
  organization(organization &&in_r) noexcept;
  /// @copydoc organization(organization &&in_r)
  organization &operator=(organization &&in_r) noexcept;
  /// @copydoc organization(organization &&in_r)
  organization(const organization &in_r) noexcept;
  /// @copydoc organization(organization &&in_r)
  organization &operator=(const organization &in_r) noexcept;

  bool operator==(const organization &in_rhs) const;
  bool operator!=(const organization &in_rhs) const;
  bool operator<(const organization &in_rhs) const;
  bool operator>(const organization &in_rhs) const;
  bool operator<=(const organization &in_rhs) const;
  bool operator>=(const organization &in_rhs) const;
};

class organization_list {
 private:
 public:
  std::map<std::string, organization> config_list;
  organization_list();
  virtual ~organization_list();

  friend void to_json(nlohmann::json &j, const organization_list &p);
  friend void from_json(const nlohmann::json &j, organization_list &p);
};

}  // namespace doodle

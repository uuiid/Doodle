#pragma once

#include <doodle_lib/doodle_lib_fwd.h>

namespace doodle {
namespace project_config {
class base_config;
}

class organization {
 private:
  friend void to_json(nlohmann::json &j, const organization &p);
  friend void from_json(const nlohmann::json &j, organization &p);

  std::unique_ptr<project_config::base_config> p_config;

 public:
  std::string org_p;
  organization();
  organization(std::string in_org_p);
  ~organization();

  static organization &get_current_organization();

  project_config::base_config &get_config() const;

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
  std::set<organization> config_list;
  organization_list();
  ~organization_list();
};

}  // namespace doodle

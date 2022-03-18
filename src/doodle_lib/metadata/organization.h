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

  project_config::base_config &get_config();
  project_config::base_config &get_config() const;

  bool operator==(const organization &in_rhs) const;
  bool operator!=(const organization &in_rhs) const;
  bool operator<(const organization &in_rhs) const;
  bool operator>(const organization &in_rhs) const;
  bool operator<=(const organization &in_rhs) const;
  bool operator>=(const organization &in_rhs) const;
};
}  // namespace doodle



#include "organization.h"
#include <doodle_lib/metadata/project.h>
namespace doodle {
organization::organization()
    : organization(std::string{}) {
}
organization::organization(std::string in_org_p)
    : org_p(std::move(in_org_p)),
      p_config(std::make_unique<project_config::base_config>()) {
}
organization::~organization() = default;

project_config::base_config& organization::get_config() {
  if (project::has_prj())
    *p_config = project::get_current().get<project_config::base_config>();
  return std::as_const(*this).get_config();
}

project_config::base_config& organization::get_config() const {
  return *p_config;
}

bool organization::operator==(const organization& in_rhs) const {
  return org_p == in_rhs.org_p;
}
bool organization::operator!=(const organization& in_rhs) const {
  return !(*this == in_rhs);
}
bool organization::operator<(const organization& in_rhs) const {
  return org_p < in_rhs.org_p;
}
bool organization::operator>(const organization& in_rhs) const {
  return in_rhs < *this;
}
bool organization::operator<=(const organization& in_rhs) const {
  return !(in_rhs < *this);
}
bool organization::operator>=(const organization& in_rhs) const {
  return !(*this < in_rhs);
}

void to_json(nlohmann::json& j, const organization& p) {
  j["name"] = p.org_p;
  j["ptr"]  = *p.p_config;
}
void from_json(const nlohmann::json& j, organization& p) {
  j.at("name").get_to(p.org_p);
  j.at("ptr").get_to(*p.p_config);
}

}  // namespace doodle

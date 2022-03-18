

#include "organization.h"
#include <doodle_lib/metadata/project.h>
#include <doodle_lib/core/core_set.h>
namespace doodle {
organization::organization()
    : organization(std::string{}) {
}
organization::organization(std::string in_org_p)
    : org_p(std::move(in_org_p)),
      p_config(std::make_unique<project_config::base_config>()) {
}
organization::~organization() = default;

organization& organization::get_current_organization() {
  static const organization def_value{};
  if (project::has_prj()) {
    auto& list = project::get_current().get<organization_list>().config_list;
    if (auto l_it = list.find(core_set::getSet().organization_name;
                              l_it != list.end())) {
      return *l_it;
    }
  }
  return def_value;
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

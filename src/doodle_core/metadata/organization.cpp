

#include "organization.h"
#include <doodle_core/metadata/project.h>
#include <core/core_set.h>
namespace doodle {
organization::organization()
    : organization(std::string{}) {
}
organization::organization(std::string in_org_p)
    : org_p(std::move(in_org_p)),
      p_config(std::make_unique<project_config::base_config>()) {
}
organization::~organization() = default;

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
organization::organization(organization&& in_r) noexcept            = default;
organization& organization::operator=(organization&& in_r) noexcept = default;

organization::organization(const organization& in_r) noexcept {
  this->org_p    = in_r.org_p;
  this->p_config = std::make_unique<project_config::base_config>(*in_r.p_config);
}
organization& organization::operator=(const organization& in_r) noexcept {
  this->org_p    = in_r.org_p;
  this->p_config = std::make_unique<project_config::base_config>(*in_r.p_config);
  return *this;
}

void to_json(nlohmann::json& j, const organization& p) {
  j["name"] = p.org_p;
  j["ptr"]  = *p.p_config;
}
void from_json(const nlohmann::json& j, organization& p) {
  j.at("name").get_to(p.org_p);
  j.at("ptr").get_to(*p.p_config);
}

organization_list::organization_list()  = default;
organization_list::~organization_list() = default;

void to_json(nlohmann::json& j, const organization_list& p) {
  j["map"] = p.config_list;
}
void from_json(const nlohmann::json& j, organization_list& p) {
  j.at("map").get_to(p.config_list);
}

}  // namespace doodle

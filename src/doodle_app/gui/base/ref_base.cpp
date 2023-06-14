#include "ref_base.h"

#include <doodle_core/core/doodle_lib.h>
#include <doodle_core/core/util.h>
#include <doodle_core/metadata/metadata.h>
namespace doodle::gui {

edit_interface::~edit_interface() = default;

gui_cache_name_id::gui_cache_name_id(const std::string &in_name) : name_id(), name() {
  auto l_size = in_name.size();

  name_id     = fmt::format(
      "{}{}{}", in_name, (in_name.find_first_of('#') != std::string::npos) ? ""s : "##"s,
      doodle_lib::Get().ctx().get<identifier>()
  );
  name = {name_id.c_str(), l_size};
  // name = k_v.substr(0, l_size);
}

const char *gui_cache_name_id::operator*() const noexcept { return name_id.c_str(); }
gui_cache_name_id::gui_cache_name_id(gui_cache_name_id &&in_r) noexcept {
  auto l_size = in_r.name.size();
  name_id     = std::move(in_r.name_id);
  name        = {name_id.c_str(), l_size};
}
gui_cache_name_id &gui_cache_name_id::operator=(gui_cache_name_id &&in_r) noexcept {
  auto l_size = in_r.name.size();
  name_id     = std::move(in_r.name_id);
  name        = {name_id.c_str(), l_size};
  return *this;
}
gui_cache_name_id::gui_cache_name_id(const gui_cache_name_id &in_r) noexcept {
  auto l_size = in_r.name.size();
  name_id     = in_r.name_id;
  name        = {name_id.c_str(), l_size};
}
gui_cache_name_id &gui_cache_name_id::operator=(const gui_cache_name_id &in_r) noexcept {
  auto l_size = in_r.name.size();
  name_id     = in_r.name_id;
  name        = {name_id.c_str(), l_size};
  return *this;
}

bool gui_cache_name_id::operator==(const gui_cache_name_id &in) const noexcept { return name_id == in.name_id; }
bool gui_cache_name_id::operator<(const gui_cache_name_id &in) const noexcept { return name_id < in.name_id; }
}  // namespace doodle::gui

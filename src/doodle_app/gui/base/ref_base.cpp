#include "ref_base.h"
#include <doodle_core/metadata/metadata.h>
#include <doodle_core/core/util.h>
namespace doodle::gui {

void edit_interface::init(const entt::handle &in) {
  init(std::vector<entt::handle>{in});
}
void edit_interface::save(const entt::handle &in) {
  save(std::vector<entt::handle>{in});
}
void edit_interface::set_modify(bool is_modify) {
  data_->is_modify = is_modify;
  data_->edited();
}
edit_interface::edit_interface()
    : data_(std::make_unique<gui_data>()) {}
void edit_interface::save(const std::vector<entt::handle> &in) {
  if (data_->is_modify)
    ranges::for_each(in, [this](const entt::handle &in_handle) {
      save_(in_handle);
    });
  data_->is_modify = false;
}
void edit_interface::init(const std::vector<entt::handle> &in) {
  data_->is_modify = false;
  if (!in.empty())
    init_(in.front());
}

edit_interface::~edit_interface() = default;

gui_cache_name_id::gui_cache_name_id(const std::string &in_name)
    : name_id(),
      name() {
  auto l_size = in_name.size();

  name_id     = fmt::format("{}{}{}", in_name, (in_name.find_first_of('#') != std::string::npos) ? ""s : "##"s, doodle::core::identifier::get().id());
  name        = {name_id.c_str(), l_size};
  // name = k_v.substr(0, l_size);
}

const char *gui_cache_name_id::operator*() const noexcept {
  return name_id.c_str();
}
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

bool gui_cache_name_id::operator==(const gui_cache_name_id &in) const noexcept {
  return name_id == in.name_id;
}
bool gui_cache_name_id::operator<(const gui_cache_name_id &in) const noexcept {
  return name_id < in.name_id;
}
}  // namespace doodle::gui

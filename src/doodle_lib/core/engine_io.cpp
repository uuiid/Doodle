//
// Created by TD on 25-1-22.
//

#include "engine_io.h"

#include "doodle_core/core/core_set.h"

#include <magic_enum.hpp>
namespace doodle::socket_io {

query_data parse_query_data(const boost::urls::url& in_url) {
  query_data l_ret{};
  for (auto&& l_item : in_url.params()) {
    if (l_item.key == "sid" && l_item.has_value) l_ret.sid_ = from_uuid_str(l_item.value);
    if (l_item.key == "transport" && l_item.has_value) {
      l_ret.transport_ = magic_enum::enum_cast<transport_type>(l_item.value).value_or(transport_type::unknown);
    }
    if (l_item.key == "EIO" && l_item.has_value) l_ret.EIO_ = std::stoi(l_item.value);
  }
  if (l_ret.EIO_ != 4 || l_ret.transport_ == transport_type::unknown)
    throw_exception(http_request_error{boost::beast::http::status::bad_request, "无效的请求"});
  return l_ret;
}

uuid sid_ctx::generate_sid() {
  // 加锁
  std::unique_lock l_lock{mutex_};

  auto&& [l_it, l_] = sid_time_map_.emplace(core_set::get_set().get_uuid(), std::chrono::system_clock::now());
  // 清除超时的sid
  auto l_now        = std::chrono::system_clock::now();
  for (auto it = sid_time_map_.begin(); it != sid_time_map_.end();) {
    if (l_now - it->second > handshake_data_.ping_timeout_)
      it = sid_time_map_.erase(it);
    else
      ++it;
  }
  return l_it->first;
}
bool sid_ctx::is_sid_timeout(const uuid& in_sid) const {
  // 加锁
  std::shared_lock l_lock{mutex_};
  return sid_time_map_.find(in_sid) != sid_time_map_.end() &&
         std::chrono::system_clock::now() - sid_time_map_.at(in_sid) > handshake_data_.ping_timeout_;
}
void sid_ctx::update_sid_time(const uuid& in_sid) {
  auto l_now = std::chrono::system_clock::now();
  // 加锁
  std::shared_lock l_lock{mutex_};
  sid_time_map_[in_sid] = l_now;
}

}  // namespace doodle::socket_io
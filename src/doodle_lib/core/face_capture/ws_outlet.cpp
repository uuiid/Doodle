//
// Created by TD on 2026-11-6.
//

#include "ws_outlet.h"

#include <doodle_core/exception/exception.h>

#include <doodle_lib/core/face_capture/face_capture_types.h>
#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/logger/logger.h>

#include <boost/asio/co_spawn.hpp>
#include <boost/asio/consign.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/use_awaitable.hpp>

#include <fmt/format.h>
#include <nlohmann/json.hpp>

#include <cstring>
#include <deque>
#include <memory>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace doodle::face_capture {

class ws_outlet::impl : public std::enable_shared_from_this<ws_outlet::impl> {
 public:
  explicit impl(executor_type in_executor) : strand_(std::move(in_executor)) {}

  ~impl() = default;

  void attach(write_fn in_write) {
    closing_ = false;
    write_   = std::move(in_write);
    queue_.clear();
  }

  void detach() {
    closing_ = true;
    write_   = {};
    queue_.clear();
  }

  bool is_attached() const { return !closing_ && static_cast<bool>(write_); }

  void set_init_segment(std::vector<std::uint8_t> in_init_segment) {
    init_segment_ = std::move(in_init_segment);
  }

  void set_session_info(boost::uuids::uuid in_session_id, std::int32_t in_width, std::int32_t in_height) {
    session_id_ = in_session_id;
    width_      = in_width;
    height_     = in_height;
  }

  void send_hello() {
    send_json(nlohmann::json{
        {"type", "hello"}, {"session_id", fmt::format("{}", session_id_)}, {"state", to_string(state_)},
        {"width", width_}, {"height", height_},                          {"fps", k_fps}
    });

    if (init_segment_.empty()) return;
    send_json(nlohmann::json{
        {"type", "avc_init"}, {"bytes", init_segment_.size()}, {"seq_begin", 0}, {"seq_end", 0}
    });
    queue_.emplace_back(item_t{.key_ = true, .bytes_ = init_segment_});
    pump();
  }

  void send_json(nlohmann::json in_json) {
    if (!is_attached()) return;
    auto l_text = in_json.dump();
    queue_.emplace_back(item_t{
        .key_ = true, .bytes_ = std::vector<std::uint8_t>{l_text.begin(), l_text.end()}, .seq_begin_ = 0, .seq_end_ = 0
    });
    while (queue_.size() > k_max_pending) queue_.pop_front();
    pump();
  }

  void send_fragment(fragment_t in_fragment) {
    if (!is_attached() || in_fragment.data_.empty()) return;

    if (!in_fragment.key_ && has_pending_frame()) return;

    queue_.emplace_back(item_t{
        .key_       = in_fragment.key_,
        .bytes_     = std::move(in_fragment.data_),
        .seq_begin_ = in_fragment.seq_begin_,
        .seq_end_   = in_fragment.seq_end_
    });
    while (queue_.size() > k_max_pending) queue_.pop_front();
    pump();
  }

  void notify_state(std::string in_state) {
    state_ = parse_state(in_state);
    send_json(nlohmann::json{{"type", "capture_state"}, {"state", std::move(in_state)}});
  }

  bool idle() const { return !pumping_ && queue_.empty(); }

  bool pumping() const { return pumping_; }

  const executor_type& get_executor() const { return strand_; }

 private:
  struct item_t {
    bool key_{false};
    std::vector<std::uint8_t> bytes_{};
    std::uint64_t seq_begin_{0};
    std::uint64_t seq_end_{0};
  };

  static constexpr std::size_t k_max_pending{8};

  static capture_state parse_state(const std::string& in_state) {
    if (in_state == "recording") return capture_state::recording;
    if (in_state == "stopping") return capture_state::stopping;
    if (in_state == "stopped") return capture_state::stopped;
    return capture_state::idle;
  }

  bool has_pending_frame() const {
    if (pumping_) return true;
    for (auto l_it = queue_.rbegin(); l_it != queue_.rend(); ++l_it) {
      if (!l_it->key_) return true;
    }
    return false;
  }

  void pump() {
    if (pumping_ || !is_attached() || queue_.empty()) return;
    pumping_ = true;
    boost::asio::co_spawn(
        strand_, drain(), boost::asio::consign(boost::asio::detached, this->shared_from_this())
    );
  }

  boost::asio::awaitable<void> drain() {
    while (!closing_ && static_cast<bool>(write_) && !queue_.empty()) {
      auto l_item = std::move(queue_.front());
      queue_.pop_front();
      if (!(co_await write_(l_item.bytes_.data(), l_item.bytes_.size()))) {
        closing_ = true;
        break;
      }
    }
    pumping_ = false;
    if (!closing_ && !queue_.empty()) pump();
  }

  const executor_type strand_;
  write_fn write_{};
  std::deque<item_t> queue_{};
  std::vector<std::uint8_t> init_segment_{};
  boost::uuids::uuid session_id_{};
  capture_state state_{capture_state::idle};
  std::int32_t width_{0};
  std::int32_t height_{0};
  bool pumping_{false};
  bool closing_{false};
};

std::shared_ptr<ws_outlet> ws_outlet::make(executor_type in_executor) {
  return std::make_shared<ws_outlet>(std::move(in_executor));
}

ws_outlet::ws_outlet(executor_type in_executor) : impl_ptr_(std::make_shared<impl>(std::move(in_executor))) {}

ws_outlet::~ws_outlet() = default;

void ws_outlet::attach(write_fn in_write) { impl_ptr_->attach(std::move(in_write)); }

void ws_outlet::detach() { impl_ptr_->detach(); }

bool ws_outlet::is_attached() const { return impl_ptr_->is_attached(); }

void ws_outlet::set_init_segment(std::vector<std::uint8_t> in_init_segment) {
  impl_ptr_->set_init_segment(std::move(in_init_segment));
}

void ws_outlet::set_session_info(boost::uuids::uuid in_session_id, std::int32_t in_width, std::int32_t in_height) {
  impl_ptr_->set_session_info(in_session_id, in_width, in_height);
}

void ws_outlet::send_hello() { impl_ptr_->send_hello(); }

void ws_outlet::send_event(nlohmann::json in_json) { impl_ptr_->send_json(std::move(in_json)); }

void ws_outlet::send_fragment(fragment_t in_fragment) { impl_ptr_->send_fragment(std::move(in_fragment)); }

void ws_outlet::notify_state(std::string in_state) { impl_ptr_->notify_state(std::move(in_state)); }

bool ws_outlet::idle() const { return impl_ptr_->idle(); }

bool ws_outlet::pumping() const { return impl_ptr_->pumping(); }

const ws_outlet::executor_type& ws_outlet::get_executor() const { return impl_ptr_->get_executor(); }

}  // namespace doodle::face_capture

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
#include <boost/asio/post.hpp>
#include <boost/asio/use_awaitable.hpp>

#include <atomic>
#include <cstring>
#include <deque>
#include <fmt/format.h>
#include <memory>
#include <nlohmann/json.hpp>
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
    attached_.store(is_attached_locked());
    idle_.store(true);
  }

  void detach() {
    closing_ = true;
    write_   = {};
    queue_.clear();
    attached_.store(false);
    idle_.store(true);
  }

  bool is_attached() const { return attached_.load(); }

  void set_init_segment(std::vector<std::uint8_t> in_init_segment) { init_segment_ = std::move(in_init_segment); }

  void set_session_info(boost::uuids::uuid in_session_id, std::int32_t in_width, std::int32_t in_height) {
    session_id_ = in_session_id;
    width_      = in_width;
    height_     = in_height;
  }

  void send_hello() {
    post([self = this->shared_from_this()] {
      self->send_json(
          nlohmann::json{
              {"type", "hello"},
              {"session_id", fmt::format("{}", self->session_id_)},
              {"state", self->state_},
              {"width", self->width_},
              {"height", self->height_},
              {"fps", k_fps}
          }
      );

      if (self->init_segment_.empty()) return;
      self->send_json(
          nlohmann::json{{"type", "avc_init"}, {"bytes", self->init_segment_.size()}, {"seq_begin", 0}, {"seq_end", 0}}
      );
      self->queue_.emplace_back(item_t{.key_ = true, .bytes_ = self->init_segment_});
      self->pump();
    });
  }

  void send_event(nlohmann::json in_json) {
    post([self = this->shared_from_this(), in_json = std::move(in_json)] { self->send_json(std::move(in_json)); });
  }

  void send_fragment(fragment_t in_fragment) {
    post([self = this->shared_from_this(), in_fragment = std::move(in_fragment)]() mutable {
      self->send_fragment_json(std::move(in_fragment));
    });
  }

  bool idle() const { return idle_.load(); }

  const executor_type& get_executor() const { return strand_; }

 private:
  struct item_t {
    bool key_{false};
    bool binary_{true};
    std::vector<std::uint8_t> bytes_{};
    std::uint64_t seq_begin_{0};
    std::uint64_t seq_end_{0};
  };

  static constexpr std::size_t k_max_pending{8};

  template <typename F>
  void post(F&& in_fn) {
    boost::asio::post(strand_, std::forward<F>(in_fn));
  }

  bool is_attached_locked() const { return !closing_ && static_cast<bool>(write_); }

  void send_json(nlohmann::json in_json) {
    if (!is_attached_locked()) return;
    auto l_text = in_json.dump();
    queue_.emplace_back(
        item_t{
            .key_       = true,
            .binary_    = false,
            .bytes_     = std::vector<std::uint8_t>{l_text.begin(), l_text.end()},
            .seq_begin_ = 0,
            .seq_end_   = 0
        }
    );
    while (queue_.size() > k_max_pending) queue_.pop_front();
    idle_.store(false);
    pump();
  }

  void send_fragment_json(fragment_t in_fragment) {
    if (!is_attached_locked() || in_fragment.data_.empty()) return;

    if (!in_fragment.key_ && has_pending_frame()) return;

    queue_.emplace_back(
        item_t{
            .key_       = in_fragment.key_,
            .binary_    = true,
            .bytes_     = std::move(in_fragment.data_),
            .seq_begin_ = in_fragment.seq_begin_,
            .seq_end_   = in_fragment.seq_end_
        }
    );
    while (queue_.size() > k_max_pending) queue_.pop_front();
    idle_.store(false);
    pump();
  }

  bool has_pending_frame() const {
    if (pumping_) return true;
    for (auto l_it = queue_.rbegin(); l_it != queue_.rend(); ++l_it) {
      if (!l_it->key_) return true;
    }
    return false;
  }

  void pump() {
    if (pumping_ || !is_attached_locked() || queue_.empty()) return;
    pumping_ = true;
    boost::asio::co_spawn(strand_, drain(), boost::asio::consign(boost::asio::detached, this->shared_from_this()));
  }

  boost::asio::awaitable<void> drain() {
    while (!closing_ && static_cast<bool>(write_) && !queue_.empty()) {
      auto l_item = std::move(queue_.front());
      queue_.pop_front();
      if (!(co_await write_(l_item.binary_, l_item.bytes_.data(), l_item.bytes_.size()))) {
        closing_ = true;
        break;
      }
    }
    pumping_ = false;
    if (closing_) {
      attached_.store(false);
      queue_.clear();
    }
    if (closing_ || queue_.empty()) {
      idle_.store(true);
    } else {
      pump();
    }
  }

  const executor_type strand_;
  std::atomic_bool attached_{false};
  std::atomic_bool idle_{true};
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

void ws_outlet::send_event(nlohmann::json in_json) { impl_ptr_->send_event(std::move(in_json)); }

void ws_outlet::send_fragment(fragment_t in_fragment) { impl_ptr_->send_fragment(std::move(in_fragment)); }

bool ws_outlet::idle() const { return impl_ptr_->idle(); }

const ws_outlet::executor_type& ws_outlet::get_executor() const { return impl_ptr_->get_executor(); }

}  // namespace doodle::face_capture

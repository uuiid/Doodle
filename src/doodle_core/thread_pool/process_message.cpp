#include "process_message.h"

#include <doodle_core/core/doodle_lib.h>
#include <doodle_core/lib_warp/boost_fmt_rational.h>
#include <doodle_core/logger/logger.h>

#include "core/app_base.h"
#include <magic_enum.hpp>
#include <range/v3/all.hpp>
#include <range/v3/range.hpp>
#include <spdlog/spdlog.h>

namespace doodle {
namespace details {
template <class Mutex>
class process_message_sink : public spdlog::sinks::base_sink<Mutex> {
  static constexpr std::int32_t l_n{10};

public:
  logger_Buffer buffer_;

  process_message_sink() {
    for (auto&& i : buffer_.loggers_) {
      i.fill('\0');
    }
  }

private:
protected:
  void sink_it_(const spdlog::details::log_msg& msg) override {
    // 获取未使用的缓冲区

    logger_Buffer* l_buffer = &buffer_;

    // 测试是否可以转换为进度
    if (msg.payload.starts_with("progress") && msg.payload.size() > 8) {
      std::string_view l_str = std::string_view{msg.payload.begin(), msg.payload.end()}.substr(9);
      auto l_pos             = l_str.find('/');
      if (l_pos != std::string::npos) {
        std::int32_t l_num{};
        std::int32_t l_den{};
        auto l_result  = std::from_chars(l_str.data(), l_str.data() + l_pos, l_num);
        auto l_result2 = std::from_chars(l_str.data() + l_pos + 1, l_str.data() + l_str.size(), l_den);

        if (l_den != 0 && l_result.ec == std::errc{} && l_result2.ec == std::errc{}) {
          l_buffer->progress_ = {l_num, l_den};
        }
      }
      return;
    }
    rational_int l_progress                     = l_buffer->progress_;
    chrono::sys_time_pos::duration l_extra_time = l_buffer->extra_time_;

    // 格式化
    spdlog::memory_buf_t formatted;
    spdlog::sinks::base_sink<Mutex>::formatter_->format(msg, formatted);
    if (l_buffer->state_ == process_message::state::wait || l_buffer->state_ == process_message::state::pause) {
      l_buffer->state_      = process_message::state::run;
      l_buffer->start_time_ = chrono::system_clock::now();
    }

    switch (msg.level) {
      case spdlog::level::level_enum::debug:
      case spdlog::level::level_enum::info:
      case spdlog::level::level_enum::warn:
      case spdlog::level::level_enum::err:
      case spdlog::level::level_enum::critical: {
        auto&& l_logg = l_buffer->loggers_[msg.level];
        std::rotate(l_logg.rbegin(), l_logg.rbegin() + formatted.size(), l_logg.rend());
        std::copy(formatted.begin(), formatted.begin() + formatted.size(), l_logg.begin());
        l_progress += {1, boost::numeric_cast<std::int32_t>(std::pow(l_n, std::clamp(5 - msg.level, 0, 5)))};
        l_buffer->end_str_.fill(' ');
        std::copy_n(std::begin(formatted),
                    std::clamp(formatted.size(), std::size_t{0}, l_buffer->end_str_.size()),
                    l_buffer->end_str_.begin());
        for (auto&& i : l_buffer->end_str_) {
          if (i == '\n') i = '\0';
        }

        break;
      }
      case spdlog::level::level_enum::off:
        l_buffer->end_time_ = chrono::system_clock::now();
        l_buffer->progress_ = {1, 1};
        if (auto l_enum = magic_enum::enum_cast<process_message::state>(fmt::to_string(msg.payload));
          l_enum.has_value()) {
          if (l_buffer->state_ == process_message::state::run && *l_enum == process_message::state::pause)
            l_extra_time += chrono::system_clock::now() - chrono::sys_time_pos{l_buffer->start_time_};
          l_buffer->state_ = l_enum.value();
        }
        break;

      case spdlog::level::level_enum::n_levels:
        break;
    }
    switch (msg.level) {
      case spdlog::level::level_enum::trace:
      case spdlog::level::level_enum::debug:
      case spdlog::level::level_enum::info:
      case spdlog::level::level_enum::warn:
        break;
      case spdlog::level::level_enum::err:
      case spdlog::level::level_enum::critical:
        l_buffer->state_ = process_message::state::fail;
        l_buffer->end_time_ = chrono::system_clock::now();
        break;
      case spdlog::level::level_enum::off:
      case spdlog::level::level_enum::n_levels:
        break;
    }

    if (l_progress > 1) --l_progress;

    l_buffer->progress_   = l_progress;
    l_buffer->extra_time_ = l_extra_time;
  }

  void flush_() override {
  }
};
} // namespace details
using process_message_sink_mt = details::process_message_sink<std::mutex>;

process_message::process_message(std::string in_name) : data_(std::make_shared<data_t>()) {
  data_->p_name    = std::move(in_name);
  data_->p_name_id = fmt::format("{}##{}", data_->p_name, fmt::ptr(this));

  data_->p_logger = g_logger_ctrl().make_log(data_->p_name);
  data_->sink_    = std::make_shared<process_message_sink_mt>();
  data_->p_logger->sinks().emplace_back(data_->sink_);
  data_->scoped_connection_ = app_base::Get().on_stop.connect([this]() { this->aborted(); });
}

const std::string& process_message::get_name() const { return data_->p_name; }
logger_ptr process_message::logger() const { return data_->p_logger; }


const details::logger_Buffer::log_str& process_message::level_log(const level::level_enum in_level) const {
  return data_->sink_->buffer_.loggers_[in_level];
}

const details::logger_Buffer::log_end_str& process_message::message_back() const {
  return data_->sink_->buffer_.end_str_;
}

rational_int process_message::get_progress() const {
  return data_->sink_->buffer_.progress_;
}

process_message::state process_message::get_state() const {
  return data_->sink_->buffer_.state_;
}

chrono::sys_time_pos::duration process_message::get_time() const {
  chrono::sys_time_pos::duration l_out = data_->sink_->buffer_.extra_time_;
  chrono::sys_time_pos l_start         = data_->sink_->buffer_.start_time_;
  chrono::sys_time_pos l_end           = data_->sink_->buffer_.end_time_;
  switch (data_->sink_->buffer_.state_) {
    case state::wait:
    case state::pause:
      break;
    case state::run:
      l_out += chrono::system_clock::now() - l_start;
      break;
    case state::fail:
    case state::success:
      l_out += l_end - l_start;
  }
  return l_out;
}


const std::string& process_message::get_name_id() const { return data_->p_name_id; }
} // namespace doodle
#pragma once

#include <doodle_core/doodle_core_fwd.h>

#include <boost/rational.hpp>
#include <boost/signals2.hpp>
namespace doodle {

using rational_int = boost::rational<std::size_t>;

class DOODLE_CORE_API process_message {
  friend void DOODLE_CORE_API to_json(nlohmann::json& nlohmann_json_j, const process_message& nlohmann_json_t);
  friend void DOODLE_CORE_API from_json(const nlohmann::json& nlohmann_json_j, process_message& nlohmann_json_t);

 public:
  enum level {
    info    = 0,
    warning = 1,
  };
  enum state {
    success = 1,
    fail    = 2,
    wait    = 3,
    run     = 4
  };

 private:
  chrono::sys_time_pos p_time;
  std::optional<chrono::sys_time_pos> p_end;
  std::string p_err;
  std::string p_log;
  std::string p_str_end;
  std::string p_name;
  std::string p_name_id;
  state p_state;
  rational_int p_progress;
  std::mutex _mutex;

 public:
  process_message();

  process_message(process_message&&) noexcept;
  process_message& operator=(process_message&&) noexcept;
  process_message(const process_message&) noexcept;
  process_message& operator=(const process_message&) noexcept;

  [[nodiscard]] const std::string& get_name() const;
  [[nodiscard]] const std::string& get_name_id() const;
  void set_name(const std::string& in_string);

  void progress_step(const rational_int& in_rational_int);
  void progress_clear();
  void message(const std::string& in_string, const level& in_level_enum = level::warning);
  [[nodiscard]] const std::string& message_back() const;

  void set_state(state in_state);
  [[nodiscard]] std::string_view err() const;
  [[nodiscard]] std::string_view log() const;

  [[nodiscard]] rational_int get_progress() const;
  [[nodiscard]] inline std::double_t get_progress_f() const {
    return boost::rational_cast<std::double_t>(get_progress() * rational_int{100});
  };
  [[nodiscard]] const state& get_state() const;
  [[nodiscard]] chrono::sys_time_pos::duration get_time() const;

  [[nodiscard]] inline bool is_run() const { return get_state() == state::run; }
  [[nodiscard]] inline bool is_wait() const { return get_state() == state::wait; }
  [[nodiscard]] inline bool is_success() const { return get_state() == state::success; }
  [[nodiscard]] inline bool is_fail() const { return get_state() == state::fail; }

  inline void aborted() const {
    if (aborted_function) aborted_function();
  };
  std::function<void()> aborted_function;
};

}  // namespace doodle

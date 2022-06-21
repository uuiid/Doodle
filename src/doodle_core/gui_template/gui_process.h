//
// Created by TD on 2022/6/21.
//

#pragma once

#include <doodle_core/doodle_core_fwd.h>

namespace doodle {
enum class process_state : std::uint8_t {
  run = 1,
  succeed,
  fail
};

template <typename Process_t>
class process_warp_t {
#define DOODLE_TYPE_HASE_MFN(mfn_name)                           \
  template <typename type_t, typename = void>                    \
  struct has_##mfn_name##_fun : std::false_type {};              \
  template <typename type_t>                                     \
  struct has_##mfn_name##_fun<                                   \
      type_t,                                                    \
      std::void_t<decltype(std::declval<type_t&>().mfn_name())>> \
      : std::true_type {                                         \
  };

  DOODLE_TYPE_HASE_MFN(init)
  DOODLE_TYPE_HASE_MFN(succeeded)
  DOODLE_TYPE_HASE_MFN(failed)
  DOODLE_TYPE_HASE_MFN(aborted)

#undef DOODLE_TYPE_HASE_MFN

 protected:
  std::unique_ptr<void, void (*)(void*)> process_attr;

  static void delete_gui_process_t(void* in_ptr) {
    delete static_cast<Process_t*>(in_ptr);
  };

  enum class state : std::uint8_t {
    uninitialized = 0,
    running,
    paused,
    succeeded,
    failed,
    aborted,
    finished,
    rejected
  };
  state current{state::uninitialized};

  template <typename Target                                     = Process_t,
            std::enable_if_t<has_init_fun<Target>::value, bool> = false>
  auto next(std::integral_constant<state, state::uninitialized>)
      -> decltype(std::declval<Target>().init(), void()) {
    auto&& l_gui = *static_cast<Process_t*>(process_attr.get());
    l_gui.init();
    current = state::running;
  }
  template <typename Target                                      = Process_t,
            std::enable_if_t<!has_init_fun<Target>::value, bool> = false>
  auto next(std::integral_constant<state, state::uninitialized>)
      -> decltype(void(), void()) {
    auto&& l_gui = *static_cast<Process_t*>(process_attr.get());
    current      = state::running;
  }

  template <typename Target = Process_t>
  auto next(std::integral_constant<state, state::running>)
      -> decltype(std::declval<Target>().update(), void()) {
    auto&& l_gui = *static_cast<Process_t*>(process_attr.get());
    switch (l_gui.update()) {
      case process_state::run:
        break;
      case process_state::succeed:
        current = state::succeeded;
        break;
      case process_state::fail:
        current = state::finished;
        break;
      default:
        break;
    }
  }

  template <typename Target                                          = Process_t,
            std::enable_if_t<has_succeeded_fun<Target>::value, bool> = false>
  auto next(std::integral_constant<state, state::succeeded>)
      -> decltype(std::declval<Target>().succeeded(), void()) {
    auto&& l_gui = *static_cast<Process_t*>(process_attr.get());
    l_gui.succeeded();
    current = state::finished;
  }
  template <typename Target                                           = Process_t,
            std::enable_if_t<!has_succeeded_fun<Target>::value, bool> = false>
  auto next(std::integral_constant<state, state::succeeded>)
      -> decltype(void(), void()) {
    auto&& l_gui = *static_cast<Process_t*>(process_attr.get());
    current      = state::finished;
  }

  template <typename Target                                       = Process_t,
            std::enable_if_t<has_failed_fun<Target>::value, bool> = false>
  auto next(std::integral_constant<state, state::failed>)
      -> decltype(std::declval<Target>().failed(), void()) {
    auto&& l_gui = *static_cast<Process_t*>(process_attr.get());
    l_gui.failed();
    current = state::rejected;
  }

  template <typename Target                                        = Process_t,
            std::enable_if_t<!has_failed_fun<Target>::value, bool> = false>
  auto next(std::integral_constant<state, state::failed>)
      -> decltype(void(), void()) {
    auto&& l_gui = *static_cast<Process_t*>(process_attr.get());
    current      = state::rejected;
  }

  template <typename Target                                        = Process_t,
            std::enable_if_t<has_aborted_fun<Target>::value, bool> = false>
  auto next(std::integral_constant<state, state::aborted>)
      -> decltype(std::declval<Target>().aborted(), void()) {
    auto&& l_gui = *static_cast<Process_t*>(process_attr.get());
    current      = state::rejected;
    l_gui.aborted();
  }

  template <typename Target                                         = Process_t,
            std::enable_if_t<!has_aborted_fun<Target>::value, bool> = false>
  auto next(std::integral_constant<state, state::aborted>)
      -> decltype(void(), void()) {
    auto&& l_gui = *static_cast<Process_t*>(process_attr.get());
    current      = state::rejected;
  }

  void succeed() {
    if (alive()) {
      current = state::succeeded;
    }
  }

  void fail() {
    if (alive()) {
      current = state::failed;
    }
  }
  void pause() {
    if (current == state::running) {
      current = state::paused;
    }
  }
  void unpause() {
    if (current == state::paused) {
      current = state::running;
    }
  }

 public:
  template <typename... Args>
  explicit process_warp_t(Args&&... in_args)
      : process_attr(
            new Process_t{std::forward<Args>(in_args)...},
            &delete_gui_process_t) {}
  explicit process_warp_t(std::unique_ptr<Process_t>&& in_ptr)
      : process_attr(std::move(in_ptr)){};

  virtual ~process_warp_t() = default;

  void abort(const bool immediately = false) {
    if (alive()) {
      current = state::aborted;

      if (immediately) {
        (*this)();
      }
    }
  }
  [[nodiscard]] bool is_uninitialized() const {
    return current == state::uninitialized;
  }

  [[nodiscard]] bool alive() const {
    return current == state::running || current == state::paused;
  }
  [[nodiscard]] bool finished() const {
    return current == state::finished;
  }
  [[nodiscard]] bool paused() const {
    return current == state::paused;
  }
  [[nodiscard]] bool rejected() const {
    return current == state::rejected;
  }
  virtual process_state operator()() {
    process_state l_state{process_state::run};
    switch (current) {
      case state::uninitialized:
        next(std::integral_constant<state, state::uninitialized>{});
        break;
      case state::running:
        next(std::integral_constant<state, state::running>{});
        break;
      default:
        // suppress warnings
        break;
    }

    // if it's dead, it must be notified and removed immediately
    switch (current) {
      case state::succeeded: {
        next(std::integral_constant<state, state::succeeded>{});
        l_state = process_state::succeed;
      } break;
      case state::failed: {
        next(std::integral_constant<state, state::failed>{});
        l_state = process_state::fail;
      } break;
      case state::aborted: {
        next(std::integral_constant<state, state::aborted>{});
        l_state = process_state::fail;
      } break;
      default:
        // suppress warnings
        break;
    }
    return l_state;
  };
};

template <typename Gui_Process>
using gui_warp_t = process_warp_t<Gui_Process>;

template <typename Rear_Process>
using rear_warp_t = process_warp_t<Rear_Process>;

template <typename Lambda_Process, typename = void>
class lambda_process_warp_t : private Lambda_Process {
 public:
  template <typename... Args>
  lambda_process_warp_t(Args&&... in_args) : Lambda_Process{std::forward<Args>(in_args)...} {};

  process_state update();
};

template <typename Lambda_Process>
class lambda_process_warp_t<
    Lambda_Process,
    std::enable_if_t<
        std::is_same_v<
            typename std::invoke_result<Lambda_Process>::type, void>>>
    : private Lambda_Process {
 public:
  template <typename... Args>
  lambda_process_warp_t(Args&&... in_args) : Lambda_Process{std::forward<Args>(in_args)...} {};

  process_state update() {
    Lambda_Process::operator()();
    return process_state::succeed;
  }
};

template <typename Lambda_Process>
class lambda_process_warp_t<
    Lambda_Process,
    std::enable_if_t<
        std::is_same_v<
            typename std::invoke_result<Lambda_Process>::type, process_state>>>
    : private Lambda_Process {
 public:
  template <typename... Args>
  lambda_process_warp_t(Args&&... in_args) : Lambda_Process{std::forward<Args>(in_args)...} {};

  process_state update() {
    return Lambda_Process::operator()();
  }
};
namespace detail {

template <typename IO_Context, typename Rear_Process>
class gui_to_rear_warp_t {
  IO_Context& io_con_p;
  rear_warp_t<Rear_Process> warp_process{};
  std::future<process_state> future_;

  std::function<process_state()> fun_sub{};

 public:
  template <typename... Args>
  explicit gui_to_rear_warp_t(IO_Context& in_io, Args&&... in_args)
      : io_con_p(in_io),
        warp_process(std::forward<Args>(in_args)...),
        future_(),
        fun_sub() {}

  void init() {
    fun_sub = [this]() mutable {
      auto l_s = this->warp_process();
      switch (l_s) {
        case process_state::run: {
          future_ = boost::asio::post(io_con_p, std::packaged_task<process_state()>{this->fun_sub});
          break;
        }
        case process_state::succeed:
        case process_state::fail:
        default:
          break;
      }
      return l_s;
    };
    future_ = boost::asio::post(io_con_p, std::packaged_task<process_state()>{fun_sub});
  }

  process_state update() {
    process_state l_state{process_state::run};
    /// \brief 过程指针有效, 状态为未初始化, 未来无效
    if (future_.valid()) {
      switch (future_.wait_for(0ns)) {
        case std::future_status::ready: {
          try {
            l_state = future_.get();
          } catch (const doodle_error& error) {
            DOODLE_LOG_ERROR(error.what());
            l_state = process_state::fail;
          }
        } break;
        default:
          break;
      }
    }
    return l_state;
  }
};

struct gui_process_wrap_handler;
using instance_type  = std::unique_ptr<void, void (*)(void*)>;
using next_type      = std::unique_ptr<gui_process_wrap_handler>;
using abort_fn_type  = void(gui_process_wrap_handler&, bool);
using update_fn_type = bool(gui_process_wrap_handler&);
struct gui_process_wrap_handler {
  instance_type instance;
  abort_fn_type* abort;
  update_fn_type* update;
  next_type next;
};
template <typename Gui_Process2>
static void delete_gui_process_t(void* in_ptr) {
  delete static_cast<Gui_Process2*>(in_ptr);
};
template <typename Gui_Process2>
static void abort(gui_process_wrap_handler& handler, const bool immediately) {
  static_cast<Gui_Process2*>(handler.instance.get())->abort(immediately);
}
template <typename Gui_Process2>
static bool update(gui_process_wrap_handler& handler) {
  auto&& l_process = *static_cast<Gui_Process2*>(handler.instance.get());

  switch (l_process()) {
    case process_state::run: {
    } break;
    case process_state::succeed: {
      if (handler.next) {
        handler = std::move(*handler.next);
      }
    } break;
    case process_state::fail:
    default:
      return true;
      break;
  }
  return false;
}

}  // namespace detail
class gui_process_t {
 private:
  using instance_type            = detail::instance_type;
  using next_type                = detail::next_type;
  using gui_process_wrap_handler = detail::gui_process_wrap_handler;

  next_type handle;
  next_type* auxiliary_next;

  template <typename type_t, typename... Args>
  gui_process_t& _post_(Args&&... in_args) {
    auto l_next = instance_type{
        new type_t{std::forward<Args>(in_args)...},
        &detail::delete_gui_process_t<type_t>};
    auxiliary_next->reset(new gui_process_wrap_handler{
        std::move(l_next),
        &detail::abort<type_t>,
        &detail::update<type_t>,
        nullptr});
    auxiliary_next = &((*auxiliary_next)->next);
    return *this;
  };

 public:
  explicit gui_process_t()
      : handle{nullptr},
        auxiliary_next(&handle) {}

  template <typename type_t, typename... Args>
  gui_process_t& then(Args&&... in_args) {
    return _post_<gui_warp_t<type_t>>(std::forward<Args>(in_args)...);
  };
  template <typename Func>
  gui_process_t& then(Func&& func) {
    return _post_<gui_warp_t<lambda_process_warp_t<Func>>>(std::forward<Func>(func));
  };
  template <typename type_t, typename IO_Context, typename... Args>
  gui_process_t& post(IO_Context& in_io, Args&&... in_args) {
    return _post_<rear_warp_t<detail::gui_to_rear_warp_t<
        IO_Context, type_t>>>(in_io, std::forward<Args>(in_args)...);
  };
  template <typename IO_Context, typename Func>
  gui_process_t& post(IO_Context& in_io, Func&& func) {
    return _post_<rear_warp_t<detail::gui_to_rear_warp_t<
        IO_Context, lambda_process_warp_t<Func>>>>(in_io, std::forward<Func>(func));
  };
  // 提交时的渲染过程
  bool operator()() {
    return handle->update(*handle);
  }
  void abort(bool in_abort = false) {
    handle->abort(*handle, in_abort);
  }
};
}  // namespace doodle

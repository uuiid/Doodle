//
// Created by TD on 2022/6/20.
//
#include <doodle_lib/doodle_lib_all.h>
#include <doodle_lib/app/app.h>
#include <doodle_core/core/core_sig.h>
#include <doodle_core/metadata/metadata_cpp.h>
#include <doodle_core/metadata/time_point_wrap.h>
#include <doodle_core/time_tool/work_clock.h>
#include <doodle_lib/gui/strand_gui.h>
#include <doodle_core/time_tool/work_clock.h>

#include <catch.hpp>
#include <catch2/catch_approx.hpp>

namespace doodle {
template <typename Executor, typename CompletionToken>
auto async_gui_work(const Executor& ex, CompletionToken&& token) {
  return boost::asio::async_initiate<CompletionToken, bool(void)>(
      [&](auto&& in_token) {
        ex.post(in_token, boost::asio::get_associated_allocator(in_token));
      },
      token
  );
}
template <typename Executor, typename CompletionToken>
auto async_work(const Executor& ex, CompletionToken&& token) {
  return boost::asio::async_initiate<CompletionToken, bool(void)>(
      [&](auto&& in_token) {
        ex.post(in_token, boost::asio::get_associated_allocator(in_token));
      },
      token
  );
}
}  // namespace doodle

class test_1 : public ::doodle::process_handy_tools {
  std::int32_t p_{};
  std::int32_t p_max{10};

 public:
  explicit test_1(std::int32_t in_int) : p_(in_int){};

  void init() {
    DOODLE_LOG_INFO(" init {}", p_);
  }
  void succeeded() {
    DOODLE_LOG_INFO(" init {}", p_);
  };
  void failed() {
    DOODLE_LOG_INFO(" init {}", p_);
  };
  void aborted() {
    DOODLE_LOG_INFO(" init {}", p_);
  };
  void update() {
    DOODLE_LOG_INFO(" init {}", p_);
    if (p_ < p_max) {
      ++p_;
    } else {
      succeed();
    }
  };
};

TEST_CASE("test gui strand2") {
  doodle::app l_app{};
  doodle::async_gui_work(
      doodle::g_io_context().get_executor(),
      []() -> bool { return true; }
  );
  //  auto l_work = doodle::async_gui_work(
  //      doodle::g_io_context().get_executor(),
  //      std::function<bool()>{[]() -> bool {}});
}
TEST_CASE("test gui strand") {
  doodle::app l_app{};
  doodle::strand_gui l_gui{doodle::g_io_context().get_executor()};

  boost::asio::post(l_gui, doodle::make_process_adapter<test_1>(l_gui, 2).next<test_1>(l_gui, 5).next(l_gui, []() {
                                                                                                  DOODLE_LOG_INFO("end");
                                                                                                })
                               .next<test_1>(doodle::g_io_context().get_executor(), 10)
                               .next(doodle::g_io_context().get_executor(), [&l_app]() {
                                 DOODLE_LOG_INFO("end");
                                 l_app.stop();
                               }));

  ;
  //  doodle::gui_process_t l_process{};
  //  l_process
  //      .then<test_1>(1)
  //      .post<test_1>(doodle::g_io_context(), 2)
  //      .then([]() {
  //        DOODLE_LOG_INFO("end");
  //      })
  //      .post(doodle::g_io_context(), [&]() {
  //        DOODLE_LOG_INFO("end");
  //        l_gui.stop();
  //      });
  //  l_gui.show(std::move(l_process));

  //  doodle::process_warp_t<test_1> l_test{std::make_unique<test_1>(1)};
  //  boost::asio::post(l_gui, []() -> bool {
  //    DOODLE_LOG_INFO("dasd");
  //    return false;
  //  });
  //  boost::asio::post(l_gui, std::packaged_task<bool()>{[]() -> bool {
  //                      DOODLE_LOG_INFO("dasd");
  //                      return false;
  //                    }});
  l_app.run();
}

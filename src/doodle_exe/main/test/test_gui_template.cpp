//
// Created by TD on 2022/6/20.
//
#include <doodle_lib/doodle_lib_all.h>
#include <doodle_lib/app/app.h>
#include <doodle_core/core/core_sig.h>
#include <doodle_core/metadata/metadata_cpp.h>
#include <doodle_core/metadata/time_point_wrap.h>
#include <doodle_lib/core/work_clock.h>


#include <catch.hpp>
#include <catch2/catch_approx.hpp>

namespace doodle {

}

class test_1 {
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
  doodle::process_state update() {
    DOODLE_LOG_INFO(" init {}", p_);
    if (p_ < p_max) {
      ++p_;
      return doodle::process_state::run;
    } else {
      return doodle::process_state::succeed;
    }
  };
};

namespace doodle {




}  // namespace doodle

TEST_CASE("test gui strand") {
  boost::asio::io_context l_context{};
  doodle::strand_gui l_gui{l_context.get_executor()};
  doodle::gui_process_t l_process{};
  l_process
      .then<test_1>(1)
      .post<test_1>(l_context, 2)
      .then([]() {
        DOODLE_LOG_INFO("end");
      })
      .post(l_context, [&]() {
        DOODLE_LOG_INFO("end");
        l_gui.stop();
      });
  l_gui.show(std::move(l_process));

  //  doodle::process_warp_t<test_1> l_test{std::make_unique<test_1>(1)};
  //  boost::asio::post(l_gui, []() -> bool {
  //    DOODLE_LOG_INFO("dasd");
  //    return false;
  //  });
  //  boost::asio::post(l_gui, std::packaged_task<bool()>{[]() -> bool {
  //                      DOODLE_LOG_INFO("dasd");
  //                      return false;
  //                    }});
  l_context.run();
}

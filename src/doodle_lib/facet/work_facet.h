//
// Created by td_main on 2023/8/21.
//
#pragma once
#include <boost/asio.hpp>
namespace doodle {

class work_facet {
  using signal_set     = boost::asio::signal_set;
  using signal_set_ptr = std::shared_ptr<signal_set>;

  static constexpr auto name{"work"};

 public:
  work_facet()  = default;
  ~work_facet() = default;
  bool post();
  void add_program_options();

 private:
  std::shared_ptr<boost::asio::executor_work_guard<boost::asio::io_context::executor_type>> guard_;

  signal_set_ptr signal_set_{};
};

}  // namespace doodle

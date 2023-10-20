//
// Created by td_main on 2023/8/15.
//

#pragma once
#include <boost/asio.hpp>
namespace doodle {

class server_facet {

 public:
  server_facet()  = default;
  ~server_facet() = default;

  bool post();
  void add_program_options(){};

 private:
  std::shared_ptr<boost::asio::executor_work_guard<boost::asio::io_context::executor_type>> guard_;
};

}  // namespace doodle

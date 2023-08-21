//
// Created by td_main on 2023/8/21.
//
#include <boost/asio.hpp>
namespace doodle {

class work_facet {
  static constexpr auto name{"work"};

 public:
  work_facet()  = default;
  ~work_facet() = default;
  bool post();
  void add_program_options();

 private:
  std::shared_ptr<boost::asio::executor_work_guard<boost::asio::io_context::executor_type>> guard_;
};

}  // namespace doodle

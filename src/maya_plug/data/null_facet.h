//
// Created by TD on 2022/10/13.
//

#include <doodle_core/core/app_facet.h>

#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/io_context.hpp>
namespace doodle::maya_plug {

class null_facet {
  std::optional<boost::asio::executor_work_guard<boost::asio::io_context::executor_type>> work_lock{};

 public:
  null_facet();

  const std::string& name() const noexcept;
  inline bool post() { return true; };
  void deconstruction();
  void add_program_options(){};
};

}  // namespace doodle::maya_plug

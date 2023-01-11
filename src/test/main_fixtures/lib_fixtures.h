

#pragma once

#include <doodle_core/doodle_core.h>
#include <doodle_core/metadata/metadata.h>
#include <doodle_core/metadata/time_point_wrap.h>
#include <doodle_core/metadata/user.h>

#include <boost/asio/io_context.hpp>

#include <memory>
#include <string>

struct lib_fixtures {
  lib_fixtures();
  virtual ~lib_fixtures();

  doodle::doodle_lib doodle_lib_attr{};
};

struct run_subprocess {
  boost::asio::io_context& io;

  std::shared_ptr<boost::process::child> child;

  std::shared_ptr<boost::process::async_pipe> out_attr;
  std::shared_ptr<boost::process::async_pipe> err_attr;
  std::shared_ptr<boost::asio::streambuf> out_str{};
  std::shared_ptr<boost::asio::streambuf> err_str{};

  bool is_stop{};

  explicit run_subprocess(boost::asio::io_context& in_io);
  void run(const std::string& in_run_fun);

  void read_(
      const std::shared_ptr<boost::process::async_pipe>& in_pipe, const std::shared_ptr<boost::asio::streambuf>& in_str
  );
};

namespace doodle {
inline std::ostream& boost_test_print_type(std::ostream& ostr, database const& right) {
  ostr << "id: " << right.get_id() << " uuid: " << right.uuid();

  return ostr;
}
inline std::ostream& boost_test_print_type(std::ostream& ostr, user const& right) {
  ostr << "name: " << right.get_name() << "(" << right.get_enus() << ")";

  return ostr;
}
}  // namespace doodle
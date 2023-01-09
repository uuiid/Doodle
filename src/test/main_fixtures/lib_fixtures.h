

#pragma once

#include <doodle_core/doodle_core.h>

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

  explicit run_subprocess(boost::asio::io_context& in_io);
  void run(const std::string& in_run_fun);

  static void read_(
      const std::shared_ptr<boost::process::async_pipe>& in_pipe, const std::shared_ptr<boost::asio::streambuf>& in_str
  );
};
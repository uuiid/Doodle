//
// Created by teXiao on 2020/11/19.
//

#include <doodle_global.h>
#include <core_global.h>
#include <future>
#include <boost/asio.hpp>

class QProgressDialog;

DOODLE_NAMESPACE_S

class updataManager {
 public:
  updataManager &operator=(const updataManager &s) = delete;
  updataManager(const updataManager &s) = delete;

  static updataManager &get();
  void addQueue(std::future<bool> &f, QProgressDialog *t);
  void run();

 private:

  std::vector<std::pair<std::future<bool>, QProgressDialog *>> p_updataFtpQueue;
  std::unique_ptr<std::thread> p_thread_;
  std::future<void> p_async_ret;
  std::mutex p_mtx;
  std::unique_ptr<boost::asio::high_resolution_timer> p_timer_;
  updataManager();
  void chickQueue();
  void run_();
};
DOODLE_NAMESPACE_E

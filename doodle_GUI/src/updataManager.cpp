//
// Created by teXiao on 2020/11/19.
//

#include "updataManager.h"
#include <QtWidgets/QProgressDialog>
#include <boost/asio/high_resolution_timer.hpp>
DOODLE_NAMESPACE_S
updataManager &doodle::updataManager::get() {
  static updataManager manager{};
  return manager;
}
updataManager::updataManager() :
    p_updataFtpQueue(),
    p_thread_(),
    p_mtx(),
    p_async_ret(),
    p_timer_(){

}
void updataManager::chickQueue() {
  for (const auto &item : p_updataFtpQueue) {
    if (item.first.wait_for(std::chrono::microseconds(1)) == std::future_status::ready) {
      item.second->setValue(100);
//      item.second->cancel();
    } else {
      if (item.second->value() < 100)
        item.second->setValue(item.second->value() + 1);
    }
  }
  {//这个域中保护变量
    std::lock_guard<std::mutex> tt(p_mtx);
    p_updataFtpQueue.erase(
        std::remove_if(p_updataFtpQueue.begin(), p_updataFtpQueue.end(),
                       [this](std::pair<std::future<bool>, QProgressDialog *> &part) {
                         return part.first.wait_for(std::chrono::microseconds(10)) == std::future_status::ready;
                       }), p_updataFtpQueue.end()
    );
  }
  if (!p_updataFtpQueue.empty()) {
    p_timer_->expires_after(std::chrono::microseconds{100});
    p_timer_->async_wait([this](boost::system::error_code ec) {if(!ec) this->chickQueue(); });
  }
}
void updataManager::run() {
  if (!p_async_ret.valid())
    p_async_ret = std::async(std::launch::async, [this]() { this->run_(); });

  if (p_async_ret.wait_for(std::chrono::microseconds(1)) == std::future_status::ready) {
    p_async_ret = std::async(std::launch::async, [this]() { this->run_(); });
  }
//  p_thread_ = std::make_unique<std::async()>(&updataManager::run_,&updataManager::get());
}
void updataManager::run_() {
  boost::asio::io_context io_context;
  p_timer_ = std::make_unique<boost::asio::high_resolution_timer>(io_context);
  p_timer_->expires_from_now(std::chrono::microseconds{100});
  p_timer_->async_wait([this](boost::system::error_code ec) {if(!ec) this->chickQueue(); });
  io_context.run();
}
void updataManager::addQueue(std::future<bool> &f, QProgressDialog *t) {
  std::lock_guard<std::mutex> tt(p_mtx);
//  std::future<bool> ttl;
//  QProgressDialog *ggg;
//  std::pair<std::future<bool>, QProgressDialog *> tes{std::move(f),t};
  p_updataFtpQueue.emplace_back(std::move(f),t);
}
DOODLE_NAMESPACE_E
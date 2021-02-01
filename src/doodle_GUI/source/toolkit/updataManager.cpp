/*
 * @Author: your name
 * @Date: 2020-11-19 15:17:00
 * @LastEditTime: 2020-11-27 12:01:07
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\doodle_GUI\src\updataManager.cpp
 */
//
// Created by teXiao on 2020/11/19.
//

#include "updataManager.h"
#include <loggerlib/Logger.h>
#include <QtWidgets/QProgressDialog>
#include <QtCore/QTimer>
#include <thread>
DOODLE_NAMESPACE_S
updataManager &doodle::updataManager::get() {
  static updataManager manager{};
  return manager;
}
updataManager::updataManager()
    : p_updataFtpQueue(), p_timer_(new QTimer(this)) {
  connect(p_timer_, &QTimer::timeout, this, &updataManager::chickQueue);
}
void updataManager::chickQueue() {
  for (const auto &item : p_updataFtpQueue) {
    try {
      if (item.first.wait_for(std::chrono::microseconds(1)) ==
          std::future_status::ready) {
        item.second->maximum();
        item.second->setValue(100);
        // std::this_thread::sleep_for(std::chrono::microseconds(300));
        item.second->close();
        item.second->deleteLater();
      } else {
        if (item.second->value() < 99)
          item.second->setValue(item.second->value() + 1);
      }
    } catch (const std::exception &e) {
      item.second->setValue(100);
      item.second->close();
      item.second->deleteLater();
      DOODLE_LOG_WARN(e.what());
    }
  }
  p_updataFtpQueue.erase(
      std::remove_if(
          p_updataFtpQueue.begin(), p_updataFtpQueue.end(),
          [this](std::pair<std::future<bool>, QProgressDialog *> &part) {
            return part.first.wait_for(std::chrono::microseconds(10)) ==
                   std::future_status::ready;
          }),
      p_updataFtpQueue.end());
  if (p_updataFtpQueue.empty()) {
    p_timer_->stop();
  }
}
void updataManager::run() {
  if (!p_timer_->isActive()) {
    p_timer_->start();
  }
}

bool updataManager::empty() const { return p_updataFtpQueue.empty(); }

void updataManager::addQueue(std::future<bool> &f, QProgressDialog *t) {
  p_updataFtpQueue.emplace_back(std::move(f), t);
}
void updataManager::addQueue(std::future<bool> &f, const std::string &lableText,
                             int maxValue) {
  p_timer_->setInterval(maxValue);
  auto pro = new QProgressDialog();
  pro->setLabelText(QString::fromStdString(lableText));
  pro->setMinimum(0);
  pro->setMaximum(100);
  pro->setValue(1);
  p_updataFtpQueue.emplace_back(std::move(f), pro);
  pro->show();
}
DOODLE_NAMESPACE_E
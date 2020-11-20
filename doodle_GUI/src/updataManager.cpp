//
// Created by teXiao on 2020/11/19.
//

#include "updataManager.h"
#include <QtWidgets/QProgressDialog>
#include <QtCore/QTimer>
DOODLE_NAMESPACE_S
updataManager &doodle::updataManager::get() {
  static updataManager manager{};
  return manager;
}
updataManager::updataManager() :
    p_updataFtpQueue(),
    p_timer_(new QTimer(this)){
  connect(p_timer_,&QTimer::timeout,
          this,&updataManager::chickQueue);
}
void updataManager::chickQueue() {
  for (const auto &item : p_updataFtpQueue) {
    if (item.first.wait_for(std::chrono::microseconds(1)) == std::future_status::ready) {
      item.second->maximum();
      item.second->setValue(100);
      item.second->close();
      item.second->deleteLater();
    } else {
      if (item.second->value() < 99)
        item.second->setValue(item.second->value() + 1);
    }
  }
    p_updataFtpQueue.erase(
        std::remove_if(p_updataFtpQueue.begin(), p_updataFtpQueue.end(),
                       [this](std::pair<std::future<bool>, QProgressDialog *> &part) {
                         return part.first.wait_for(std::chrono::microseconds(10)) == std::future_status::ready;
                       }), p_updataFtpQueue.end()
    );
  if (p_updataFtpQueue.empty()) {
    p_timer_->stop();
  }
}
void updataManager::run() {
  if (!p_timer_->isActive()){
    p_timer_->start();
  }
}
void updataManager::run_() {
}
void updataManager::addQueue(std::future<bool> &f, QProgressDialog *t) {
  p_updataFtpQueue.emplace_back(std::move(f),t);
}
void updataManager::addQueue(std::future<bool> &f, const std::string &lableText, int maxValue) {
  p_timer_->setInterval(maxValue);
  auto pro = new QProgressDialog();
  pro->setLabelText(QString::fromStdString(lableText));
  pro->setMinimum(0);
  pro->setMaximum(100);
  pro->setValue(1);
  p_updataFtpQueue.emplace_back(std::move(f),pro);
  pro->show();
}
DOODLE_NAMESPACE_E
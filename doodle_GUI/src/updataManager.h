/*
 * @Author: your name
 * @Date: 2020-11-19 15:17:00
 * @LastEditTime: 2020-11-27 11:26:54
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\doodle_GUI\src\updataManager.h
 */
//
// Created by teXiao on 2020/11/19.
//

#include <doodle_global.h>
#include <core_global.h>
#include <future>
#include <boost/asio.hpp>

class QProgressDialog;

DOODLE_NAMESPACE_S

class updataManager : public QObject {
  Q_OBJECT
 public:
  updataManager &operator=(const updataManager &s) = delete;
  updataManager(const updataManager &s) = delete;

  static updataManager &get();
  void addQueue(std::future<bool> &f, QProgressDialog *t);
  void addQueue(std::future<bool> &f, const std::string &lableText,
                int maxValue);
  void run();
  bool empty() const;

 private:
  std::vector<std::pair<std::future<bool>, QProgressDialog *>> p_updataFtpQueue;
  QTimer *p_timer_;

  updataManager();
  void chickQueue();
};
DOODLE_NAMESPACE_E

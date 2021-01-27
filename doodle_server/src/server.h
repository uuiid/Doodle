/*
 * @Author: your name
 * @Date: 2020-12-12 19:17:30
 * @LastEditTime: 2020-12-16 11:02:55
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\doodle_server\server.h
 */

#pragma once
#include <DoodleServer_global.h>

DOODLE_NAMESPACE_S

enum class fileOptions {
  getInfo      = 0,
  createFolder = 1,
  update       = 2,
  down         = 3,
};

class Handler {
 public:
  Handler();

  void operator()(zmq::context_t* context);
  // zmq::multipart_t handleMessage(zmq::multipart_t* message);
};

DOODLE_NAMESPACE_E
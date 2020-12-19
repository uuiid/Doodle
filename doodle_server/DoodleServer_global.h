/*
 * @Author: your name
 * @Date: 2020-12-12 19:25:24
 * @LastEditTime: 2020-12-14 10:19:35
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\doodle_server\DoodleServer.h
 */

#pragma once

#include <boost/network/include/http/server.hpp>

namespace boost {

namespace filesystem {
class path;
};

namespace iostreams {
class mapped_file_source;
};
};  // namespace boost

#define DOODLE doodle
#define DOODLE_NAMESPACE_S namespace DOODLE {
#define DOODLE_NAMESPACE_E \
  }                        \
  ;

DOODLE_NAMESPACE_S
using path_ptr       = std::shared_ptr<boost::filesystem::path>;
using mappedFile_ptr = std::shared_ptr<boost::iostreams::mapped_file_source>;

class Handler;
using Server = boost::network::http::server<Handler>;
class fileSystem;
using fileSystem_ptr = std::shared_ptr<fileSystem>;

DOODLE_NAMESPACE_E
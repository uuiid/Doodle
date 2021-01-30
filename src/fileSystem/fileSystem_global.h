/*
 * @Author: your name
 * @Date: 2020-09-02 09:53:52
 * @LastEditTime: 2020-11-29 17:27:04
 * @LastEditors: your name
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\fileSystem\fileSystem_global.h
 */
#pragma once

#include <boost/filesystem/path.hpp>
#include <QtCore/qglobal.h>
#include <memory>
#include <string>

#if defined(FILESYSTEM_LIBRARY)
#define DSYSTEM_API __declspec(dllexport)
#else
#define DSYSTEM_API __declspec(dllimport)
#endif

#define DSYSTEM_S namespace doSystem {
#define DSYSTEM_E }

namespace boost::filesystem {
class path;
}

DSYSTEM_S
namespace fileSys = boost::filesystem;
class DfileSyntem;
class ftpSession;
typedef std::shared_ptr<ftpSession> ftpSessionPtr;
using dstring = std::string;
using dpath   = boost::filesystem::path;
const static uint64_t off{8000000};
DSYSTEM_E
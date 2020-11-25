#pragma once

#include <QtCore/qglobal.h>
#include <memory>
#include <string>
#include <boost/filesystem/path.hpp>

#if defined (FILESYSTEM_LIBRARY)
#   define DSYSTEM_API Q_DECL_EXPORT
#else
#   define DSYSTEM_API Q_DECL_IMPORT
#endif

#define DSYSTEM_S namespace doSystem {
#define DSYSTEM_E }

DSYSTEM_S

class DfileSyntem;
class ftpSession;
typedef std::shared_ptr<ftpSession> ftpSessionPtr;
using dstring = std::string;
using dpath = boost::filesystem::path;

DSYSTEM_E
#pragma once

#include <QtCore/qglobal.h>
#include <memory>
#include <string>
#if defined (FTP_LIBRARY)
#   define FTP_EXPORT Q_DECL_EXPORT
#else
#   define FTP_EXPORT Q_DECL_IMPORT
#endif

#define FTPSPACE_S namespace doFtp {
#define FTPSPACE_E }

FTPSPACE_S

class ftphandle;
class ftpSession;
typedef std::shared_ptr<ftpSession> ftpSessionPtr;
using dstring = std::string;
FTPSPACE_E
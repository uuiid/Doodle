#pragma once

#include <QtCore/qglobal.h>
#if defined (FTP_LIBRARY)
#   define FTP_EXPORT Q_DECL_EXPORT
#else
#   define FTP_EXPORT Q_DECL_IMPORT
#endif

#define FTPSPACE_S namespace doFtp {
#define FTPSPACE_E }


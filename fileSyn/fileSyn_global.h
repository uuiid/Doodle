#ifndef FILESYN_GLOBAL_H
#define FILESYN_GLOBAL_H

#include <QtCore/qglobal.h>
//#include <QtGlobal>

#if defined(FILESYN_LIBRARY)
#  define FILESYN_EXPORT Q_DECL_EXPORT
#else
#  define FILESYN_EXPORT Q_DECL_IMPORT
#endif

#define DNAMESPACE_S namespace doFileSyn {
#define DNAMESPACE_E }
#endif // FILESYN_GLOBAL_H


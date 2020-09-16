#pragma once


#include <QtCore/qglobal.h>
//#include <QtGlobal>

#if defined(FILESYN_LIBRARY)
#  define FILESYN_EXPORT Q_DECL_EXPORT
#else
#  define FILESYN_EXPORT Q_DECL_IMPORT
#endif

#define SYN_NAMESPACE_S namespace doFileSyn {
#define DNAMESPACE_E }



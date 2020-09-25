#pragma once


#include <QtCore/qglobal.h>
#include <QSharedPointer>

#if defined(PINYIN_LIBRARY)
#  define PINYIN_EXPORT Q_DECL_EXPORT
#else
#  define PINYIN_EXPORT Q_DECL_IMPORT
#endif

#define PINYIN_NAMESPACE_S namespace dopinyin {
#define DNAMESPACE_E }

PINYIN_NAMESPACE_S

class convert;

typedef QSharedPointer<convert> convertPtr;

DNAMESPACE_E

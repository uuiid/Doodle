#pragma once

#include <string>
#include <memory>
#include <vector>

#if defined(PINYIN_LIBRARY)
#define PINYIN_EXPORT __declspec(dllexport)
#else
#define PINYIN_EXPORT __declspec(dllimport)
#endif

#define PINYIN_NAMESPACE_S namespace dopinyin {
#define DNAMESPACE_E }

PINYIN_NAMESPACE_S
class convert;
DNAMESPACE_E

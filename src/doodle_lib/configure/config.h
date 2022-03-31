//
// Created by TD on 2022/3/31.
//

#pragma once
// clang-format off
//#define Doodle_VERSION_MAJOR 3
//#define Doodle_VERSION_MINOR 4
//#define Doodle_VERSION_PATCH 19
//#define Doodle_VERSION_TWEAK 31
// clang-format on





#if defined _WIN32
//
//
//#ifndef _WIN32_WINNT
//#define _WIN32_WINNT 0x0A00
//#else
//#undef _WIN32_WINNT
//#define _WIN32_WINNT 0x0A00
//#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#else
#undef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#else
#undef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#ifndef _SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING
#define _SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING
#else
#undef _SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING
#define _SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING
#endif

#ifndef _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
#else
#undef _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
#endif

#elif defined __linux__ // _WIN32

#endif // __linux__


#include <boost/current_function.hpp>
#ifndef SPDLOG_FUNCTION
#define SPDLOG_FUNCTION static_cast<const char *>( BOOST_CURRENT_FUNCTION )
#else
#undef SPDLOG_FUNCTION
#define SPDLOG_FUNCTION static_cast<const char *>( BOOST_CURRENT_FUNCTION )
#endif

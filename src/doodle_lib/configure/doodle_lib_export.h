//
// Created by TD on 2022/8/4.
//

#pragma once

#ifdef DOODLE_LIB_STATIC_DEFINE
#define DOODLELIB_API
#define DOODLE_LIB_NO_EXPORT
#else
#ifndef DOODLELIB_API
#ifdef doodle_lib_EXPORTS
/* We are building this library */
#define DOODLELIB_API
#else
/* We are using this library */
#define DOODLELIB_API
#endif
#endif

#ifndef DOODLE_LIB_NO_EXPORT
#define DOODLE_LIB_NO_EXPORT
#endif
#endif

#ifndef DOODLE_LIB_DEPRECATED
#define DOODLE_LIB_DEPRECATED __declspec(deprecated)
#endif

#ifndef DOODLE_LIB_DEPRECATED_EXPORT
#define DOODLE_LIB_DEPRECATED_EXPORT DOODLELIB_API DOODLE_LIB_DEPRECATED
#endif

#ifndef DOODLE_LIB_DEPRECATED_NO_EXPORT
#define DOODLE_LIB_DEPRECATED_NO_EXPORT DOODLE_LIB_NO_EXPORT DOODLE_LIB_DEPRECATED
#endif

#if 0 /* DEFINE_NO_DEPRECATED */
#ifndef DOODLE_LIB_NO_DEPRECATED
#define DOODLE_LIB_NO_DEPRECATED
#endif
#endif

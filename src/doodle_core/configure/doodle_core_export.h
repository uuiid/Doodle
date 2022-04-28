//
// Created by TD on 2022/4/28.
//

#pragma once

#ifdef DOODLE_CORE_STATIC_DEFINE
#define DOODLE_CORE_EXPORT
#define DOODLE_CORE_NO_EXPORT
#else
#ifndef DOODLE_CORE_EXPORT
#ifdef doodle_core_EXPORTS
/* We are building this library */
#define DOODLE_CORE_EXPORT __declspec(dllexport)
#else
/* We are using this library */
#define DOODLE_CORE_EXPORT __declspec(dllimport)
#endif
#endif

#ifndef DOODLE_CORE_NO_EXPORT
#define DOODLE_CORE_NO_EXPORT
#endif
#endif

#ifndef DOODLE_CORE_DEPRECATED
#define DOODLE_CORE_DEPRECATED __declspec(deprecated)
#endif

#ifndef DOODLE_CORE_DEPRECATED_EXPORT
#define DOODLE_CORE_DEPRECATED_EXPORT DOODLE_CORE_EXPORT DOODLE_CORE_DEPRECATED
#endif

#ifndef DOODLE_CORE_DEPRECATED_NO_EXPORT
#define DOODLE_CORE_DEPRECATED_NO_EXPORT DOODLE_CORE_NO_EXPORT DOODLE_CORE_DEPRECATED
#endif

#if 0 /* DEFINE_NO_DEPRECATED */
#ifndef DOODLE_CORE_NO_DEPRECATED
#define DOODLE_CORE_NO_DEPRECATED
#endif
#endif

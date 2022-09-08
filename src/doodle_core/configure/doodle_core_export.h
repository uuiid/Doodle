//
// Created by TD on 2022/4/28.
//

#pragma once

#ifdef DOODLE_CORE_STATIC_DEFINE
#define DOODLE_CORE_API
#else
#ifndef DOODLE_CORE_EXPORT
#ifdef DOODLE_CORE_API
/* We are building this library */
#define DOODLE_CORE_API __declspec(dllexport)
#else
/* We are using this library */
#define DOODLE_CORE_API __declspec(dllimport)
#endif

#endif

#endif

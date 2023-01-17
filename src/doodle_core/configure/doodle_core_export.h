//
// Created by TD on 2022/4/28.
//

#pragma once


#ifdef DOODLE_CORE_EXPORT
/* We are building this library */
// #define DOODLE_CORE_API __declspec(dllexport)
#else
/* We are using this library */
// #define DOODLE_CORE_API __declspec(dllimport)
#endif
#define DOODLE_CORE_API

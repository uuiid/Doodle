//
// Created by TD on 2022/4/28.
//

#pragma once


#ifdef DOODLE_APP_EXPORT
/* We are building this library */
#define DOODLE_APP_API __declspec(dllexport)
#else
/* We are using this library */
#define DOODLE_APP_API __declspec(dllimport)
#endif


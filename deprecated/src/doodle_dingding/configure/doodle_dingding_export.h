//
// Created by TD on 2022/9/7.
//

#pragma once



#ifdef DOODLE_DINGDING_EXPORTS
/* We are building this library */
#define DOODLE_DINGDING_API __declspec(dllexport)
#else
/* We are using this library */
#define DOODLE_DINGDING_API __declspec(dllimport)

#endif

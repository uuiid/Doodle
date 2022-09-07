//
// Created by TD on 2022/9/7.
//

#pragma once

//
// Created by TD on 2022/8/4.
//

#pragma once

#ifdef DOODLE_DINGDING_STATIC_DEFINE
#define DOODLE_DINGDING_API

#else  // DOODLE_DINGDING_STATIC_DEFINE


#ifdef DOODLE_DINGDING_EXPORTS
/* We are building this library */
#define DOODLE_DINGDING_API __declspec(dllexport)
#else
/* We are using this library */
#define DOODLE_DINGDING_API __declspec(dllimport)

#endif


#endif  // DOODLE_DINGDING_STATIC_DEFINE

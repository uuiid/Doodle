// #include

#pragma once

#if defined(MOTIONGLOBAL_LIBRARY)
#define MOTIONGLOBAL_API __declspec(dllexport)
#else
#define MOTIONGLOBAL_API __declspec(dllimport)
#endif


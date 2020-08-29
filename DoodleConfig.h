#pragma once

#define fileSyn_VERSION_MAJOR 
#define fileSyn_VERSION_MINOR 

#if defined _WIN32
#define EXPORT __declspec(dllexport)
#define IMPORT __declspec(dllimport)
#endif // defined

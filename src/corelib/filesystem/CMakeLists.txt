target_sources(
  core
  PUBLIC
  Path.h
  FileSystem.h
  fileSync.h
)

target_sources(
  core
  PRIVATE
  Path.cpp
  FileSystem.cpp
  fileSync.cpp
)
#pragma once

#include <doodle_server/DoodleServer_global.h>

#include <doodle_server/source/Project.h>

DOODLE_NAMESPACE_S

class Seting {
 private:
  Seting();
  ~Seting();

 public:
  Seting(const Seting&) = delete;
  Seting& operator=(const Seting&) = delete;

  static Seting& Get() noexcept;

  void init() const;
};

DOODLE_NAMESPACE_E
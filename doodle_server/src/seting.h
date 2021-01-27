#pragma once

#include <DoodleServer_global.h>

#include <src/Project.h>

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
#pragma once

#include <corelib/core_global.h>
#include <boost/signals2.hpp>

namespace doodle {
class LongTerm {
 private:
 public:
  LongTerm() : progress(),
               messagResult(),
               finished(){};
  virtual ~LongTerm()= default;

  boost::signals2::signal<void(int)> progress;
  boost::signals2::signal<void(const std::string& message)> messagResult;
  boost::signals2::signal<void()> finished;
};

}  // namespace doodle
#pragma once

#include <DoodleLib/DoodleLib_fwd.h>
#include <boost/signals2.hpp>

namespace doodle {
class LongTerm {

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

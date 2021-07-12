#pragma once

#include <DoodleLib/DoodleLib_fwd.h>
#include <boost/signals2.hpp>

namespace doodle {
class long_term {

 public:
  long_term() : sig_progress(),
                sig_message_result(),
                sig_finished(){};
  virtual ~long_term()= default;

  boost::signals2::signal<void(int)> sig_progress;
  boost::signals2::signal<void(const std::string& message)> sig_message_result;
  boost::signals2::signal<void()> sig_finished;
};

}  // namespace doodle

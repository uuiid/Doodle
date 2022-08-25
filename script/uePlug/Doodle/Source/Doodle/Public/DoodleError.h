#pragma once

#include <stdexcept>
#include <string>
namespace doodle {

class DoodleError : public std::runtime_error {
 public:
  DoodleError(const std::string &err) : std::runtime_error(err){};
};
}  // namespace doodle

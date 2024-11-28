//
// Created by TD on 2022/10/8.
//

#include "get_prot.h"
// #include <boost/winapi/error_handling.hpp>
#include <doodle_core/logger/logger.h>

#include <Windows.h>
#include <iphlpapi.h>
#include <iprtrmib.h>
#include <wil/result.h>
#include <winsock.h>

namespace doodle::win {

namespace {
struct MIB_TCPTABLE_free {
 public:
  void operator()(MIB_TCPTABLE2* in) { std::free(in); }
};
}  // namespace

}  // namespace doodle::win

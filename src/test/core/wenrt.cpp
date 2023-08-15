//
// Created by TD on 2022/8/26.
//

#include "doodle_core/core/global_function.h"
#include "doodle_core/exception/exception.h"
#include "doodle_core/logger/logger.h"
#include "doodle_core/metadata/assets_file.h"
#include <doodle_core/doodle_core.h>
#include <doodle_core/pin_yin/convert.h>

#include "boost/asio/buffer.hpp"
#include "boost/asio/execution/context.hpp"
#include "boost/asio/execution_context.hpp"
#include "boost/asio/signal_set.hpp"
#include "boost/asio/windows/object_handle.hpp"
#include "boost/asio/windows/overlapped_ptr.hpp"
#include "boost/numeric/conversion/cast.hpp"
#include "boost/winapi/file_management.hpp"
#include <boost/asio.hpp>
#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/system_timer.hpp>
#include <boost/test/tools/interface.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/test/unit_test_log.hpp>

#include "fmt/core.h"
// clang-format off


#include <algorithm>
#include <atomic>
#include <cwchar>
#include <memory>
#include <sddl.h>
#include <string>
#include <thread>
#include <type_traits>
#include <unknwn.h>
#include <utility>
#include <vector>
#include <wil/result.h>


#include <windows.h>
#include <winternl.h>
#include <comutil.h>
#include <oleidl.h>
#include <ntstatus.h>
#include <cfapi.h>

#include <SearchAPI.h>  // needed for AddFolderToSearchIndexer
#include <propkey.h>      // needed for ApplyTransferStateToFile
#include <propvarutil.h>  // needed for ApplyTransferStateToFile
#include <winrt/base.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Storage.h>
#include <winrt/windows.security.cryptography.h>
#include <winrt/windows.storage.compression.h>
#include <winrt/windows.storage.provider.h>

// clang-format on

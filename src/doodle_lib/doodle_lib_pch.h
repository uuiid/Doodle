//
// Created by TD on 2021/6/17.
//

#pragma once

#include <boost/algorithm/algorithm.hpp>
#include <boost/filesystem.hpp>
#include <boost/iostreams/device/back_inserter.hpp>
#include <boost/iostreams/stream_buffer.hpp>
#include <boost/range.hpp>
#include <boost/range/adaptors.hpp>
#include <boost/range/algorithm_ext.hpp>
#include <boost/range/algorithm.hpp>
#include <boost/range/irange.hpp>

#include <chrono>
#include <codecvt>
#include <condition_variable>
#include <filesystem>
#include <fstream>
#include <functional>
#include <future>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <regex>
#include <stdexcept>
#include <string>
#include <typeindex>
#include <unordered_set>
#include <variant>
#include <vector>
#pragma warning(disable : 4251)
#pragma warning(disable : 4275)

#include <doodle_lib/doodle_macro.h>
#include <doodle_lib/libWarp/boost_locale_warp.h>
#include <doodle_lib/libWarp/boost_serialization_warp.h>
#include <doodle_lib/libWarp/boost_uuid_warp.h>
#include <doodle_lib/libWarp/cmrcWarp.h>
#include <doodle_lib/libWarp/sqlppWarp.h>
#include <doodle_lib/libWarp/std_warp.h>
#include <date/date.h>
#include <fmt/format.h>
#include <fmt/ostream.h>

#include <boost/numeric/conversion/cast.hpp>

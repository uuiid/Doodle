//
// Created by TD on 2021/6/17.
//

#pragma once

#include <chrono>
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
#include <unordered_set>
#include <variant>
#include <vector>
#include <codecvt>
#include <stdexcept>
#include <boost/filesystem.hpp>
#include <boost/iostreams/device/back_inserter.hpp>
#include <boost/iostreams/stream_buffer.hpp>
#include <boost/range.hpp>
#include <boost/range/irange.hpp>
#include <boost/range/algorithm_ext.hpp>
#pragma warning(disable : 4251)
#pragma warning(disable : 4275)

#include <DoodleLib/DoodleMacro.h>
#include <DoodleLib/libWarp/boost_locale_warp.h>
#include <DoodleLib/libWarp/boost_serialization_warp.h>
#include <DoodleLib/libWarp/boost_uuid_warp.h>
#include <DoodleLib/libWarp/cmrcWarp.h>
#include <DoodleLib/libWarp/sqlppWarp.h>
#include <DoodleLib/libWarp/std_warp.h>
#include <date/date.h>
#include <fmt/format.h>
#include <fmt/ostream.h>

#include <boost/numeric/conversion/cast.hpp>

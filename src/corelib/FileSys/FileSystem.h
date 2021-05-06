//
// Created by teXiao on 2021/4/28.
//
#pragma once

#include <corelib/core_global.h>

namespace doodle {
class CORE_API FileSystem {
    public:
    static void localCopy(const FSys::path& in_sourcePath, const FSys::path& targetPath, bool backup);
};

}  // namespace doodle
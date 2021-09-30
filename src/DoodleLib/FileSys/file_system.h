//
// Created by teXiao on 2021/4/28.
//
#pragma once

#include <DoodleLib/doodleLib_fwd.h>

namespace doodle {
class DOODLELIB_API file_system {
    public:
    static void local_copy(const FSys::path& in_sourcePath, const FSys::path& targetPath, bool backup);
};

}  // namespace doodle

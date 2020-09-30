#include "doException.h"

CORE_NAMESPACE_S

const char *doodle_notInsert::what() const noexcept
{
    std::string str("insert MySql failed : ");
    str += std::runtime_error::what();
    return str.c_str();
}

const char *doodle_notFile::what() const noexcept
{
    std::string str("file not exists : ");
    str = str + std::runtime_error::what();
    return str.c_str();
}

const char *doodle_CopyErr::what() const noexcept
{
    std::string str("copy file failed : ");
    str = str + std::runtime_error::what();
    return str.c_str();
}

const char *doodle_upload_error::what() const noexcept
{
    std::string str("not upload file : ");
    str = str + std::runtime_error::what();
    return str.c_str();
}

const char *doodle_InsertErrorInfo::what() const noexcept
{
    std::string str("insert err info :");
    str = str + std::runtime_error::what();
    return str.c_str();
}
CORE_DNAMESPACE_E
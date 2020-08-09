#include "setting.h"

setting::setting( )
{
}

setting::setting(const setting&)
{
}

setting& setting::operator=(const setting&)
{
    // TODO: 在此处插入 return 语句
}

setting& setting::GetSetting( )
{
    static setting instance;
    
    return instance;
}

setting::~setting( )
{
}
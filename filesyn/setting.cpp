#include "setting.h"

setting::setting( )
{
}


setting& setting::GetSetting( )
{
    static setting instance;
    
    return instance;
}

setting::~setting( )
{
}
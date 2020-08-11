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

void setting::setRoot(boost::filesystem::path root_)
{
    root = root_;
}

boost::filesystem::path setting::getRoot( )
{
    return root;
}

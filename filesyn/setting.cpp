#include "setting.h"

globalSetting::globalSetting( )
{
}


globalSetting& globalSetting::GetSetting( )
{
    static globalSetting instance;
    return instance;
}

globalSetting::~globalSetting( )
{
}

void globalSetting::setRoot(boost::filesystem::path root_)
{
    root = root_;
}

boost::filesystem::path globalSetting::getRoot( )
{
    return root;
}

void globalSetting::addExclude(const char& regex)
{

}

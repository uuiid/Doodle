// filesyn.h: 标准系统包含文件的包含文件
// 或项目特定的包含文件。

#pragma once


#include "setting.h"
#include <boost/filesystem.hpp>
#include <iostream>

class fileSyn
{
public:
	fileSyn( );
	~fileSyn( );
	void scanfile(boost::filesystem::path root);

private:

};
// TODO: 在此处引用程序需要的其他标头。

#pragma once

#include <boost/filesystem.hpp>
#include <boost/regex.hpp>
class setting
{
public:
	static setting& GetSetting( );
	~setting( );
	void setRoot(boost::filesystem::path root_);
	boost::filesystem::path getRoot( );
	void addExclude(const char& regex);
private:
	setting( );
	setting(const setting&) = delete;
	setting& operator = (const setting& s) = delete;

	boost::filesystem::path root;
	std::vector<boost::regex> RegexExclude;


};


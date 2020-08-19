#pragma once

#include <boost/filesystem.hpp>
#include <boost/regex.hpp>
class globalSetting
{
public:

	enum class synmethod
	{
		upload, down, ignore
	};

	enum class fileSeat
	{
		local, server
	};

	enum class compareResult
	{
		earlier, Later, nonExistent
	};

	static globalSetting& GetSetting( );
	~globalSetting( );

	void setRoot(boost::filesystem::path root_);
	boost::filesystem::path getRoot( );
	void addExclude(const char& regex);
private:
	globalSetting( );
	globalSetting(const globalSetting&) = delete;
	globalSetting& operator = (const globalSetting& s) = delete;
private:
	boost::filesystem::path root;
	std::vector<boost::regex> RegexExclude;


};



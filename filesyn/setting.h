
class setting
{
public:
	static setting& GetSetting( );
	~setting( );
private:
	setting( );
	setting(const setting&) = delete;
	setting& operator = (const setting& s) = delete;
};


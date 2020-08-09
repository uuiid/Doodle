
class setting
{
public:
	static setting& GetSetting( );
	~setting( );
private:
	setting( );
	setting(const setting&);
	setting& operator = (const setting&);
};


set DoodleName=%1
set DoodleSource=//192.168.20.89/Doodle2/build/Ninja_release/_CPack_Packages/win64/ZIP/%DoodleName%/bin


@REM ֹͣ����
net stop doodle_kitsu_supplement
@REM �����ļ�
robocopy %DoodleSource% D:/kitsu/bin /MIR /xd dist
@REM ��������
net start doodle_kitsu_supplement
@REM ����ǰ�˽���
robocopy //192.168.20.89/kitsu/dist D:/kitsu/dist /MIR
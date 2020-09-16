create table mainshot
(
	id smallint auto_increment
		primary key,
	episods smallint null,
	episodes smallint not null,
	filepath varchar(1024) null,
	version smallint null,
	filetime datetime default CURRENT_TIMESTAMP null on update CURRENT_TIMESTAMP
);


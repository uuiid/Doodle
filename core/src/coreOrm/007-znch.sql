create table znch
(
	id smallint auto_increment
		primary key,
	localname varchar(128) null,
	__ass_class__ smallint null,
	constraint __ass_class__
		unique (__ass_class__),
	constraint localname
		unique (localname),
	constraint znch_ibfk_1
		foreign key (__ass_class__) references assclass (id)
);


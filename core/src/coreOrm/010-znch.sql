create table znch
(
	id smallint auto_increment
		primary key,
	localname varchar(128) null,
	assClass_id smallint null,
	constraint assClass_id
		unique (assClass_id),
	constraint znch_ibfk_1
		foreign key (assClass_id) references assclass (id)
);


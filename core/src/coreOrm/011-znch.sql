create table znch
(
	id bigint auto_increment
		primary key,
	localname varchar(128) null,
	assClass_id bigint null,
	constraint assClass_id
		unique (assClass_id),
	constraint znch_ibfk_1
		foreign key (assClass_id) references assclass (id)
);


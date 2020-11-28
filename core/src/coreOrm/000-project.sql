create table project
(
	id bigint auto_increment,
	name varchar(128) null,
	constraint id
		unique (id)
);

alter table project
	add primary key (id);


create table project
(
	id smallint auto_increment,
	name varchar(128) null,
	constraint id
		unique (id)
);

alter table project
	add primary key (id);


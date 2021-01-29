create table if not exists user
(
	id bigint auto_increment,
	user varchar(128) not null,
	password varchar(1024) not null,
	constraint id
		unique (id)
);

alter table user
	add primary key (id);


create table episodes
(
	id smallint auto_increment,
	episodes smallint null,
	constraint episodes
		unique (episodes),
	constraint id
		unique (id)
);

alter table episodes
	add primary key (id);


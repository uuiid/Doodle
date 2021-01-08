create table if not exists episodes
(
	id bigint auto_increment,
	episodes bigint null,
	project_id bigint null,
	constraint id
		unique (id),
	constraint episodes_ibfk_1
		foreign key (project_id) references project (id)
);

create index episodes
	on episodes (episodes);

create index project_id
	on episodes (project_id);

alter table episodes
	add primary key (id);


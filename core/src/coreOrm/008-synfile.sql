create table if not exists synfile
(
	id smallint auto_increment,
	assClass_id smallint null,
	project_id smallint null,
	episodes_id smallint null,
	path varchar(4096) not null,
	constraint id
		unique (id),
	constraint synfile_ibfk_1
		foreign key (assClass_id) references assclass (id),
	constraint synfile_ibfk_2
		foreign key (project_id) references project (id),
	constraint synfile_ibfk_3
		foreign key (episodes_id) references episodes (id)
);

create index assClass_id
	on synfile (assClass_id);

create index episodes_id
	on synfile (episodes_id);

create index project_id
	on synfile (project_id);

alter table synfile
	add primary key (id);


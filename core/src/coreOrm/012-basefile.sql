create table basefile
(
	id smallint auto_increment,
	file varchar(128) null,
	fileSuffixes varchar(32) null,
	user varchar(128) null,
	version smallint null,
	_file_path_ varchar(4096) null,
	infor varchar(4096) null,
	filestate varchar(64) null,
	filetime datetime default CURRENT_TIMESTAMP null,
	assClass_id smallint null,
	assType_id smallint null,
	episodes_id smallint null,
	shots_id smallint null,
	shotClass_id smallint null,
	shotType_id smallint null,
	project_id smallint null,
	constraint id
		unique (id),
	constraint basefile_ibfk_1
		foreign key (assClass_id) references assclass (id),
	constraint basefile_ibfk_2
		foreign key (assType_id) references asstype (id),
	constraint basefile_ibfk_3
		foreign key (episodes_id) references episodes (id),
	constraint basefile_ibfk_4
		foreign key (shots_id) references shots (id),
	constraint basefile_ibfk_5
		foreign key (shotClass_id) references shotclass (id),
	constraint basefile_ibfk_6
		foreign key (shotType_id) references shottype (id),
	constraint basefile_ibfk_7
		foreign key (project_id) references project (id)
);

create index assClass_id
	on basefile (assClass_id);

create index assType_id
	on basefile (assType_id);

create index episodes_id
	on basefile (episodes_id);

create index project_id
	on basefile (project_id);

create index shotClass_id
	on basefile (shotClass_id);

create index shotType_id
	on basefile (shotType_id);

create index shots_id
	on basefile (shots_id);

alter table basefile
	add primary key (id);


create table if not exists asstype
(
	id bigint auto_increment,
	ass_type varchar(64) null,
	project_id bigint null,
	constraint id
		unique (id),
	constraint asstype_ibfk_1
		foreign key (project_id) references project (id)
);

create index project_id
	on asstype (project_id);

alter table asstype
	add primary key (id);


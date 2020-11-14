create table assdepartment
(
	id smallint auto_increment,
	ass_dep varchar(64) null,
	project_id smallint null,
	constraint id
		unique (id),
	constraint assdepartment_ibfk_1
		foreign key (project_id) references project (id)
);

create index project_id
	on assdepartment (project_id);

alter table assdepartment
	add primary key (id);


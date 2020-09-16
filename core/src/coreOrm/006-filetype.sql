create table filetype
(
	id smallint auto_increment,
	file_type varchar(64) null,
	__file_class__ smallint null,
	__ass_class__ smallint null,
	__episodes__ smallint null,
	__shot__ smallint null,
	constraint id
		unique (id),
	constraint filetype_ibfk_1
		foreign key (__file_class__) references fileclass (id),
	constraint filetype_ibfk_2
		foreign key (__ass_class__) references assclass (id),
	constraint filetype_ibfk_3
		foreign key (__episodes__) references episodes (id),
	constraint filetype_ibfk_4
		foreign key (__shot__) references shot (id)
);

create index __ass_class__
	on filetype (__ass_class__);

create index __episodes__
	on filetype (__episodes__);

create index __file_class__
	on filetype (__file_class__);

create index __shot__
	on filetype (__shot__);

alter table filetype
	add primary key (id);


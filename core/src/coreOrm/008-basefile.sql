create table basefile
(
	id smallint auto_increment,
	file varchar(128) null,
	fileSuffixes varchar(32) null,
	user varchar(128) null,
	version smallint null,
	_file_path_ varchar(1024) null,
	infor varchar(4096) null,
	filestate varchar(64) null,
	filetime datetime default CURRENT_TIMESTAMP null,
	__episodes__ smallint null,
	__shot__ smallint null,
	__file_class__ smallint null,
	__file_type__ smallint null,
	__ass_class__ smallint null,
	constraint id
		unique (id),
	constraint basefile_ibfk_1
		foreign key (__episodes__) references episodes (id),
	constraint basefile_ibfk_2
		foreign key (__shot__) references shot (id),
	constraint basefile_ibfk_3
		foreign key (__file_class__) references fileclass (id),
	constraint basefile_ibfk_4
		foreign key (__file_type__) references filetype (id),
	constraint basefile_ibfk_5
		foreign key (__ass_class__) references assclass (id)
);

create index __ass_class__
	on basefile (__ass_class__);

create index __episodes__
	on basefile (__episodes__);

create index __file_class__
	on basefile (__file_class__);

create index __file_type__
	on basefile (__file_type__);

create index __shot__
	on basefile (__shot__);

alter table basefile
	add primary key (id);


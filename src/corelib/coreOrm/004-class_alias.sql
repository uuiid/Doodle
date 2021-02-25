create table class_alias
(
	id INTEGER,
	doodleClassName TEXT not null,
	doodleAlias TEXT not null,
	classIndex int not null,
	constraint class_alias_pk
		primary key (id),
	foreign key (classIndex) references doodleConfig
);


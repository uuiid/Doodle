create table PathParserData
(
	id INTEGER not null,
	refClass TEXT not null,
	regex TEXT not null,
	prefix TEXT,
	suffix TEXT,
	method TEXT,
	arg TEXT,
	parent int,
	primary key (id),
	foreign key (parent) references PathParserData
);

create unique index PathParser_id_index
	on PathParserData (id);


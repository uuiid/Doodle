create table pathStart
(
	id INTEGER,
	rootKey TEXT not null,
	root TEXT not null,
	Parser INTEGER,
	constraint pathStart_pk
		primary key (id),
	foreign key (Parser) references PathParserData
);


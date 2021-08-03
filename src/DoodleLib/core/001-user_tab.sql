create table if not exists doodle_test.user_tab
(
	id bigint unsigned auto_increment
		primary key,
	user_name text not null,
	uuid_path text null,
	permission_group bigint default '0' not null
);

create index user_tab_id_index
	on doodle_test.user_tab (id);

create index user_tab_permission_group_index
	on doodle_test.user_tab (permission_group);


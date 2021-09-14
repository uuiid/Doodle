-- create user 'deve'@'%' identified by 'deve';
-- GRANT ALL PRIVILEGES ON *.* TO 'deve'@'%';
-- FLUSH PRIVILEGES;


DELIMITER $$
DROP PROCEDURE IF EXISTS `doodle`.`CreateIndex` $$
CREATE PROCEDURE `doodle`.`CreateIndex`
(
    given_database VARCHAR(64),
    given_table    VARCHAR(64),
    given_index    VARCHAR(64),
    given_columns  VARCHAR(64)
)
BEGIN

    DECLARE IndexIsThere INTEGER;

    SELECT COUNT(1) INTO IndexIsThere
    FROM INFORMATION_SCHEMA.STATISTICS
    WHERE table_schema = given_database
      AND   table_name   = given_table
      AND   index_name   = given_index;

    IF IndexIsThere = 0 THEN
        SET @sqlstmt = CONCAT('CREATE INDEX ',given_index,' ON ',
                              given_database,'.',given_table,' (',given_columns,')');
        PREPARE st FROM @sqlstmt;
        EXECUTE st;
        DEALLOCATE PREPARE st;
    ELSE
        SELECT CONCAT('Index ',given_index,' already exists on Table ',
                      given_database,'.',given_table) CreateindexErrorMessage;
    END IF;

END $$

DELIMITER ;



create table if not exists metadatatab
(
    id          bigint unsigned auto_increment
        primary key,
    parent      bigint unsigned                    null,
    uuidPath    text                               null,
    update_time datetime default CURRENT_TIMESTAMP not null,
    meta_type   bigint   default 0                 null,
    constraint fk_test_id
        foreign key (parent)
            references metadatatab (id)
            on delete cascade
);
call createindex('doodle','metadatatab','Metadata_id_index','id');
call createindex('doodle','metadatatab','Metadata_parent_index','parent');

# create index Metadata_id_index
#     on metadatatab (id);

# create index Metadata_parent_index
#     on metadatatab (parent);

create table if not exists usertab
(
    id               bigint unsigned auto_increment
        primary key,
    user_name        text             not null,
    uuid_path        text             null,
    permission_group bigint default 0 not null
);

call createindex('doodle','usertab','usertab_id_index','id');
call createindex('doodle','usertab','usertab_permission_group_index','permission_group');
call createindex('doodle','usertab','usertab_user_name_index','user_name(512)');
# create index usertab_id_index
#     on usertab (id);
#
# create index usertab_permission_group_index
#     on usertab (permission_group);
#
# create index usertab_user_name_index
#     on usertab (user_name(512))


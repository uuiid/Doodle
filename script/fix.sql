pragma foreign_keys = on;
.mode csv
.output D:/test4.csv

select task.id,
       substr(hex(task.uuid), 1, 8)
           || '-' || substr(hex(task.uuid), 9, 4)
           || '-' || substr(hex(task.uuid), 13, 4)
           || '-' || substr(hex(task.uuid), 17, 4)
           || '-' || substr(hex(task.uuid), 21, 12)
from main.task
         join main.entity on task.entity_id = entity.uuid
         join task_type on task.task_type_id = task_type.uuid
         join entity_asset_extend on entity.uuid = entity_asset_extend.entity_id

where (task.task_type_id not in (select task_type_asset_type_link.task_type_id
                                 from task_type_asset_type_link
                                          join asset_type on task_type_asset_type_link.asset_type_id = asset_type.uuid
                                 where (entity.entity_type_id = asset_type.uuid)) and
       entity.entity_type_id not in (select asset_type.uuid
                                     from asset_type
                                     where asset_type.name in
                                           ('Episode', 'Sequence', 'Shot', 'Edit', 'Scene',
                                            'Concept'))
          )
order by task.project_id;



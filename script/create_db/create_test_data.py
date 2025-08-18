from os import getenv
from sqlalchemy import create_engine
from sqlalchemy.orm import Session
import doodle.project_status
import doodle.organisation
import doodle.studio
import doodle.status_automation
import doodle.entity_type
import doodle.task_status
import doodle.department
import doodle.task_type
import doodle.base
import doodle.preview_background_file
import doodle.person
import doodle.metadata_descriptor
import doodle.project
import doodle.entity
import doodle.asset_instance
import doodle.preview_file
import doodle.software
# import doodle.working_file
# import doodle.output_file
import doodle.file_status
import doodle.output_type
import doodle.task
import doodle.notification
import doodle.comment
import doodle.attachment_file
import doodle.chat_message
import doodle.chat
import doodle.subscription
import doodle.preview_file
import doodle.doodle_orm
from doodle.base import BaseMixin

from sqlalchemy_utils import UUIDType
from sqlalchemy.dialects.postgresql import JSONB
from doodle.base import BaseMixin

from sqlalchemy import orm
import sqlalchemy
import json
import uuid
import datetime
import random

class working_file(BaseMixin):
    __tablename__ = "working_file"
    uuid_id: orm.Mapped[UUIDType] = orm.mapped_column(
        UUIDType(binary=True), unique=True, nullable=False, index=True
    )

    name = orm.mapped_column(sqlalchemy.String(80))
    description = orm.mapped_column(sqlalchemy.Text())
    comment = orm.mapped_column(sqlalchemy.Text())
    revision = orm.mapped_column(sqlalchemy.Integer())
    size = orm.mapped_column(sqlalchemy.Integer())
    checksum = orm.mapped_column(sqlalchemy.String(80))
    path = orm.mapped_column(sqlalchemy.Text())
    data = orm.mapped_column(sqlalchemy.Text())
    software_type = orm.mapped_column(sqlalchemy.Text())
    task_id = orm.mapped_column(UUIDType(binary=True), index=True)
    entity_id = orm.mapped_column(UUIDType(binary=True), index=True)
    person_id = orm.mapped_column(UUIDType(binary=True), index=True)


def run():
    engine = create_engine("sqlite:///D:\\kitsu_new.database")
    
    with Session(engine) as session:
        l_task : list[doodle.task.Task] = session.query(doodle.task.Task).all()
        l_work_file : list[working_file] =[]
        for i in range(4):
            for t in l_task:
                if random.randint(1, 100) % 2 == 0:
                    continue
                w = working_file()
                w.uuid_id = uuid.uuid4()
                w.name = "test"
                w.description = "test"
                w.comment = "test"
                w.revision = 1
                w.size = 1
                w.checksum = "test"
                w.path = "tset.ma" if random.randint(1, 100) % 2 == 0 else None
                w.data = None
                w.software_type = "maya"
                w.task_id = t.uuid
                w.entity_id = t.entity_id
                l_work_file.append(w)
        session.bulk_save_objects(l_work_file)
        session.commit()

if __name__ == "__main__":
    run()
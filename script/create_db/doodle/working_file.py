from sqlalchemy.orm import relationship
from sqlalchemy_utils import UUIDType

from sqlalchemy import orm
import sqlalchemy
from doodle.base import BaseMixin

from sqlalchemy.dialects.postgresql import JSONB


class WorkingFile(BaseMixin):
    """
    Describes the file related to the work done on a given task. It is
    used as source of output files published for a given entity.
    """
    __tablename__ = "working_file"
    uuid: orm.Mapped[UUIDType] = orm.mapped_column(
        UUIDType(binary=True), unique=True, nullable=False, index=True
    )
    shotgun_id = orm.mapped_column(sqlalchemy.Integer(), index=True)

    name = orm.mapped_column(sqlalchemy.String(250))
    description = orm.mapped_column(sqlalchemy.Text())
    comment = orm.mapped_column(sqlalchemy.Text())
    revision = orm.mapped_column(sqlalchemy.Integer())
    size = orm.mapped_column(sqlalchemy.Integer())
    checksum = orm.mapped_column(sqlalchemy.Integer())
    path = orm.mapped_column(sqlalchemy.String(400))
    data = orm.mapped_column(sqlalchemy.Text())

    task_id = orm.mapped_column(
        UUIDType(binary=True), sqlalchemy.ForeignKey("task.uuid"), index=True
    )
    entity_id = orm.mapped_column(
        UUIDType(binary=True), sqlalchemy.ForeignKey("entity.uuid"), index=True
    )
    person_id = orm.mapped_column(
        UUIDType(binary=True), sqlalchemy.ForeignKey("person.uuid"), index=True
    )
    software_id = orm.mapped_column(
        UUIDType(binary=True), sqlalchemy.ForeignKey("software.uuid"), index=True
    )
    outputs = relationship("OutputFile", back_populates="source_file")

    __table_args__ = (
        sqlalchemy.UniqueConstraint(
            "name", "task_id", "entity_id", "revision", name="working_file_uc"
        ),
    )

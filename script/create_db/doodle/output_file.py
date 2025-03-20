from sqlalchemy.orm import relationship
from sqlalchemy_utils import UUIDType

from sqlalchemy import orm
import sqlalchemy

from doodle.base import BaseMixin


class OutputFile(BaseMixin):
    """
    Describe a file generated from a CG artist scene. It's the result of a
    publication.
    It is linked to a working file, an entity and a task type.
    """
    __tablename__ = "output_file"

    uuid: orm.Mapped[UUIDType] = orm.mapped_column(
        UUIDType(binary=True), unique=True, nullable=False, index=True
    )
    shotgun_id = orm.mapped_column(sqlalchemy.Integer())

    name = orm.mapped_column(sqlalchemy.String(250), nullable=False)
    canceled = orm.mapped_column(sqlalchemy.Boolean(), default=False, nullable=False)
    size = orm.mapped_column(sqlalchemy.Integer())
    checksum = orm.mapped_column(sqlalchemy.String(32))
    description = orm.mapped_column(sqlalchemy.Text())
    comment = orm.mapped_column(sqlalchemy.Text())
    extension = orm.mapped_column(sqlalchemy.String(10))
    revision = orm.mapped_column(sqlalchemy.Integer(), nullable=False)
    representation = orm.mapped_column(sqlalchemy.String(20), index=True)
    nb_elements = orm.mapped_column(sqlalchemy.Integer(), default=1)
    source = orm.mapped_column(sqlalchemy.String(40))
    path = orm.mapped_column(sqlalchemy.String(400))
    data = orm.mapped_column(sqlalchemy.TEXT())

    file_status_id = orm.mapped_column(
        UUIDType(binary=True),
        sqlalchemy.ForeignKey("file_status.uuid"),
        nullable=False,
        index=True,
    )
    entity_id = orm.mapped_column(
        UUIDType(binary=True), sqlalchemy.ForeignKey("entity.uuid"), index=True
    )
    asset_instance_id = orm.mapped_column(
        UUIDType(binary=True), sqlalchemy.ForeignKey("asset_instance.uuid"), index=True
    )
    output_type_id = orm.mapped_column(
        UUIDType(binary=True), sqlalchemy.ForeignKey("output_type.uuid"), index=True
    )
    task_type_id = orm.mapped_column(
        UUIDType(binary=True), sqlalchemy.ForeignKey("task_type.uuid"), index=True
    )
    person_id = orm.mapped_column(
        UUIDType(binary=True), sqlalchemy.ForeignKey("person.uuid"), index=True
    )
    source_file_id = orm.mapped_column(
        UUIDType(binary=True), sqlalchemy.ForeignKey("working_file.uuid"), index=True
    )
    source_file = relationship("WorkingFile", back_populates="outputs")
    temporal_entity_id = orm.mapped_column(
        UUIDType(binary=True),
        sqlalchemy.ForeignKey("entity.uuid"),
        default=None,
        nullable=True,
        index=True,
    )

    __table_args__ = (
        sqlalchemy.UniqueConstraint(
            "name",
            "entity_id",
            "asset_instance_id",
            "output_type_id",
            "task_type_id",
            "temporal_entity_id",
            "representation",
            "revision",
            name="output_file_uc",
        ),
    )

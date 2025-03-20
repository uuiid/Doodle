from sqlalchemy_utils import UUIDType

from sqlalchemy import orm
import sqlalchemy

from doodle.base import BaseMixin
from zou.app.models.task_type import TaskType as ZouTaskType


class TaskType(BaseMixin):
    """
    Categorize tasks in domain areas: modeling, animation, etc.
    """
    __tablename__ = "task_type"
    uuid: orm.Mapped[UUIDType] = orm.mapped_column(
        UUIDType(binary=True), unique=True, nullable=False, index=True
    )
    name = orm.mapped_column(sqlalchemy.String(40), nullable=False)
    short_name = orm.mapped_column(sqlalchemy.String(20))
    description = orm.mapped_column(sqlalchemy.Text())
    color = orm.mapped_column(sqlalchemy.String(7), default="#FFFFFF")
    priority = orm.mapped_column(sqlalchemy.Integer, default=1, nullable=False)
    for_entity = orm.mapped_column(sqlalchemy.String(30), default="Asset")
    allow_timelog = orm.mapped_column(sqlalchemy.Boolean, default=True, nullable=False)
    archived = orm.mapped_column(sqlalchemy.Boolean(), default=False, nullable=False)
    shotgun_id = orm.mapped_column(sqlalchemy.Integer, index=True)

    department_id = orm.mapped_column(
        UUIDType(binary=True), sqlalchemy.ForeignKey("department.uuid"), index=True
    )

    __table_args__ = (
        sqlalchemy.UniqueConstraint(
            "name", "for_entity", "department_id", name="task_type_uc"
        ),
    )

    def from_zou(self, task_type: ZouTaskType):
        self.name = task_type.name
        self.short_name = task_type.short_name
        self.description = task_type.description
        self.color = task_type.color
        self.priority = task_type.priority
        self.for_entity = task_type.for_entity
        self.allow_timelog = task_type.allow_timelog
        self.archived = task_type.archived
        self.shotgun_id = task_type.shotgun_id
        self.department_id = task_type.department_id
        self.uuid = task_type.id
        return self

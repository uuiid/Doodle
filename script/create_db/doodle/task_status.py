from doodle.base import BaseMixin
from sqlalchemy.sql import expression
from sqlalchemy import orm
import sqlalchemy
from sqlalchemy_utils import UUIDType
import zou.app.models.task_status as ZouTaskStatus


class TaskStatus(BaseMixin):
    """
    Describe the state of a task. A status marked as reviewable expects a
    preview file linked to relate comment.
    """
    __tablename__ = "task_status"
    uuid_id: orm.Mapped[UUIDType] = orm.mapped_column(
        UUIDType(binary=True), unique=True, nullable=False, index=True
    )
    name = orm.mapped_column(sqlalchemy.String(40), nullable=False)
    archived = orm.mapped_column(sqlalchemy.Boolean(), default=False)
    short_name = orm.mapped_column(
        sqlalchemy.String(10), unique=True, nullable=False, index=True
    )
    description = orm.mapped_column(sqlalchemy.Text())
    color = orm.mapped_column(sqlalchemy.String(7), nullable=False)
    priority = orm.mapped_column(sqlalchemy.Integer, default=1)

    is_done = orm.mapped_column(sqlalchemy.Boolean(), default=False, index=True)
    is_artist_allowed = orm.mapped_column(sqlalchemy.Boolean(), default=True)
    is_client_allowed = orm.mapped_column(sqlalchemy.Boolean(), default=True)
    is_retake = orm.mapped_column(sqlalchemy.Boolean(), default=False)
    is_feedback_request = orm.mapped_column(sqlalchemy.Boolean(), default=False, index=True)
    is_default = orm.mapped_column(sqlalchemy.Boolean(), default=False, index=True)
    shotgun_id = orm.mapped_column(sqlalchemy.Integer)

    for_concept = orm.mapped_column(
        sqlalchemy.Boolean(), server_default=expression.false(), default=False
    )

    def from_zou(self, task_status: ZouTaskStatus.TaskStatus):
        self.name = task_status.name
        self.archived = task_status.archived
        self.short_name = task_status.short_name
        self.description = task_status.description
        self.color = task_status.color
        self.priority = task_status.priority
        self.is_done = task_status.is_done
        self.is_artist_allowed = task_status.is_artist_allowed
        self.is_client_allowed = task_status.is_client_allowed
        self.is_retake = task_status.is_retake
        self.is_feedback_request = task_status.is_feedback_request
        self.is_default = task_status.is_default
        self.shotgun_id = task_status.shotgun_id
        self.for_concept = task_status.for_concept

        self.uuid_id = task_status.id
        return self

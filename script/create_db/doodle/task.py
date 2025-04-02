from sqlalchemy_utils import UUIDType
from sqlalchemy.dialects.postgresql import JSONB
from doodle.base import BaseMixin

from sqlalchemy import orm
import sqlalchemy
import json
from zou.app.models.task import Task as ZouTask
import datetime
class Assignations(BaseMixin):
    __tablename__ = "assignations"
    task_id = orm.mapped_column(
        UUIDType(binary=True),
        sqlalchemy.ForeignKey("task.uuid"),
        nullable=False
    )
    person_id = orm.mapped_column(
        UUIDType(binary=True),
        sqlalchemy.ForeignKey("person.uuid"),
        nullable=False
    )

    def from_zou(self, assignation):
        self.task_id = assignation[0]
        self.person_id = assignation[1]
        return self
    


class Task(BaseMixin):
    """
    Describes a task done by a CG artist on an entity of the CG production.
    The task has a state and assigned to people. It handles notion of time like
    duration, start date and end date.
    """
    __tablename__ = "task"

    uuid: orm.Mapped[UUIDType] = orm.mapped_column(
        UUIDType(binary=True), unique=True, nullable=False, index=True
    )
    name = orm.mapped_column(sqlalchemy.String(80))
    description = orm.mapped_column(sqlalchemy.Text())

    priority = orm.mapped_column(sqlalchemy.Integer, nullable=False, default=0)
    difficulty = orm.mapped_column(sqlalchemy.Integer, default=3, nullable=False)
    duration = orm.mapped_column(sqlalchemy.Float, default=0, nullable=False)
    estimation = orm.mapped_column(sqlalchemy.Float, default=0, nullable=False)
    completion_rate = orm.mapped_column(sqlalchemy.Integer, nullable=False, default=0)
    retake_count = orm.mapped_column(sqlalchemy.Integer, nullable=False, default=0)
    sort_order = orm.mapped_column(sqlalchemy.Integer, nullable=False, default=0)
    start_date = orm.mapped_column(sqlalchemy.DateTime)
    due_date = orm.mapped_column(sqlalchemy.DateTime)
    real_start_date = orm.mapped_column(sqlalchemy.DateTime)
    end_date = orm.mapped_column(sqlalchemy.DateTime)
    done_date = orm.mapped_column(sqlalchemy.DateTime)
    last_comment_date = orm.mapped_column(sqlalchemy.DateTime)
    nb_assets_ready = orm.mapped_column(sqlalchemy.Integer, nullable=False, default=0)
    data = orm.mapped_column(sqlalchemy.TEXT())
    nb_drawings = orm.mapped_column(sqlalchemy.Integer, nullable=False, default=0)

    created_at = orm.mapped_column(sqlalchemy.DateTime, nullable=False, default=datetime.datetime.now(tz=datetime.timezone.utc).replace(tzinfo=None))
    updated_at = orm.mapped_column(sqlalchemy.DateTime, nullable=False, default=datetime.datetime.now(tz=datetime.timezone.utc).replace(tzinfo=None))

    shotgun_id = orm.mapped_column(sqlalchemy.Integer, nullable=False, default=0)
    last_preview_file_id = orm.mapped_column(UUIDType(binary=True))

    project_id = orm.mapped_column(
        UUIDType(binary=True), sqlalchemy.ForeignKey("project.uuid"), index=True
    )
    task_type_id = orm.mapped_column(
        UUIDType(binary=True), sqlalchemy.ForeignKey("task_type.uuid"), index=True
    )
    task_status_id = orm.mapped_column(
        UUIDType(binary=True), sqlalchemy.ForeignKey("task_status.uuid"), index=True
    )
    entity_id = orm.mapped_column(
        UUIDType(binary=True), sqlalchemy.ForeignKey("entity.uuid"), index=True
    )
    assigner_id = orm.mapped_column(
        UUIDType(binary=True), sqlalchemy.ForeignKey("person.uuid"), index=True
    )
    assignees = orm.relationship("Person", secondary=Assignations.__table__)

    __table_args__ = (
        sqlalchemy.UniqueConstraint(
            "name", "project_id", "task_type_id", "entity_id", name="task_uc"
        ),
        sqlalchemy.CheckConstraint(
            "difficulty > 0 AND difficulty < 6", name="check_difficulty"
        ),
    )

    def from_zou(self, task: ZouTask):
        self.uuid = task.id
        self.name = task.name
        self.description = task.description
        self.priority = task.priority
        self.difficulty = task.difficulty
        self.duration = task.duration
        self.estimation = task.estimation
        self.completion_rate = task.completion_rate
        self.retake_count = task.retake_count
        self.sort_order = task.sort_order
        self.start_date = task.start_date
        self.due_date = task.due_date
        self.real_start_date = task.real_start_date
        self.end_date = task.end_date
        self.done_date = task.done_date
        self.last_comment_date = task.last_comment_date
        self.nb_assets_ready = task.nb_assets_ready
        self.data = json.dumps(task.data)
        self.nb_drawings = task.nb_drawings if hasattr(task, "nb_drawings") else 0
        self.shotgun_id = task.shotgun_id
        self.last_preview_file_id = task.last_preview_file_id

        self.project_id = task.project_id
        self.task_type_id = task.task_type_id
        self.task_status_id = task.task_status_id
        self.entity_id = task.entity_id
        self.assigner_id = task.assigner_id
        self.created_at = task.created_at
        self.updated_at = task.updated_at

        return self


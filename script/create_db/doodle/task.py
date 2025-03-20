from sqlalchemy_utils import UUIDType
from sqlalchemy.dialects.postgresql import JSONB
from doodle.base import BaseMixin

from sqlalchemy import orm
import sqlalchemy

class Assignations(BaseMixin):
    __tablename__ = "assignations"
    task = orm.mapped_column(
        UUIDType(binary=True),
        sqlalchemy.ForeignKey("task.uuid"),
    )
    person = orm.mapped_column(
        UUIDType(binary=True),
        sqlalchemy.ForeignKey("person.uuid"),
    )


class Task(BaseMixin):
    """
    Describes a task done by a CG artist on an entity of the CG production.
    The task has a state and assigned to people. It handles notion of time like
    duration, start date and end date.
    """
    __tablename__ = "task"

    uuid : orm.Mapped[UUIDType] = orm.mapped_column(
        UUIDType(binary=True), unique=True, nullable=False, index=True
    )
    name = orm.mapped_column(sqlalchemy.String(80), nullable=False)
    description = orm.mapped_column(sqlalchemy.Text())

    priority = orm.mapped_column(sqlalchemy.Integer, default=0)
    difficulty = orm.mapped_column(sqlalchemy.Integer, default=3, nullable=False)
    duration = orm.mapped_column(sqlalchemy.Float, default=0)
    estimation = orm.mapped_column(sqlalchemy.Float, default=0)
    completion_rate = orm.mapped_column(sqlalchemy.Integer, default=0)
    retake_count = orm.mapped_column(sqlalchemy.Integer, default=0)
    sort_order = orm.mapped_column(sqlalchemy.Integer, default=0)
    start_date = orm.mapped_column(sqlalchemy.DateTime)
    due_date = orm.mapped_column(sqlalchemy.DateTime)
    real_start_date = orm.mapped_column(sqlalchemy.DateTime)
    end_date = orm.mapped_column(sqlalchemy.DateTime)
    done_date = orm.mapped_column(sqlalchemy.DateTime)
    last_comment_date = orm.mapped_column(sqlalchemy.DateTime)
    nb_assets_ready = orm.mapped_column(sqlalchemy.Integer, default=0)
    data = orm.mapped_column(sqlalchemy.TEXT())
    nb_drawings = orm.mapped_column(sqlalchemy.Integer, default=0)

    shotgun_id = orm.mapped_column(sqlalchemy.Integer)
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

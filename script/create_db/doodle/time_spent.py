from sqlalchemy_utils import UUIDType

from sqlalchemy import orm
import sqlalchemy
from doodle.base import BaseMixin


class TimeSpent(BaseMixin):
    """
    Describes the time spent by someone on a task.
    """

    duration = db.Column(db.Float, nullable=False)
    date = db.Column(db.Date, nullable=False)

    task_id = db.Column(
        UUIDType(binary=False), db.ForeignKey("task.id"), index=True
    )
    person_id = db.Column(
        UUIDType(binary=False), db.ForeignKey("person.id"), index=True
    )

    __table_args__ = (
        db.UniqueConstraint(
            "person_id", "task_id", "date", name="time_spent_uc"
        ),
        db.CheckConstraint("duration > 0", name="check_duration_positive"),
    )

from sqlalchemy_utils import UUIDType

from sqlalchemy import orm
import sqlalchemy
from doodle.base import BaseMixin


class DayOff(BaseMixin):
    """
    Tells that someone will have a day off this day.
    """

    date = db.Column(db.Date, nullable=False)
    end_date = db.Column(db.Date, nullable=False)
    description = db.Column(db.Text)
    person_id = db.Column(
        UUIDType(binary=False), db.ForeignKey("person.id"), index=True
    )
    __table_args__ = (
        db.UniqueConstraint("person_id", "date", name="day_off_uc"),
        db.CheckConstraint("date <= end_date", name="day_off_date_check"),
    )

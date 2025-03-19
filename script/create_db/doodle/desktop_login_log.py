from sqlalchemy_utils import UUIDType

from sqlalchemy import orm
import sqlalchemy

from doodle.base import BaseMixin


class DesktopLoginLog(BaseMixin):
    """
    Table to log all desktop session logins. The aim is to build report that
    helps validating presence form.
    """

    person_id = db.Column(
        UUIDType(binary=False),
        db.ForeignKey("person.id"),
        nullable=False,
        index=True,
    )
    date = db.Column(db.DateTime, nullable=False)

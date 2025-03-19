from doodle.base import BaseMixin

from sqlalchemy import orm
import sqlalchemy


class FileStatus(BaseMixin):
    """
    Describe the state of a given file.
    """

    name = db.Column(db.String(40), unique=True, nullable=False)
    color = db.Column(db.String(7), nullable=False)

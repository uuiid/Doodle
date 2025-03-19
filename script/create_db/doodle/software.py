from doodle.base import BaseMixin

from sqlalchemy.dialects.postgresql import JSONB

from sqlalchemy import orm
import sqlalchemy


class Software(BaseMixin):
    """
    Describes software used by working files.
    """

    name = db.Column(db.String(40), unique=True, nullable=False)
    short_name = db.Column(db.String(20), nullable=False)
    file_extension = db.Column(db.String(20), nullable=False)
    secondary_extensions = db.Column(JSONB)

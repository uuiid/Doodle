from sqlalchemy.dialects.postgresql import JSONB

from sqlalchemy import orm
import sqlalchemy

from doodle.base import BaseMixin


class DataImportError(BaseMixin):
    """
    Table to allow the storage of import errors.
    """

    event_data = db.Column(JSONB, nullable=False)
    source = db.Column(db.Enum("csv", "shotgun", name="import_source_enum"))

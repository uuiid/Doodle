from doodle.base import BaseMixin
from sqlalchemy import orm
import sqlalchemy


class CustomAction(BaseMixin):
    """
    Custom actions are HTTP links that can be activated outside of the API.
    They are mainly aimed at being used by the web frontend to allow studio
    to make custom HTTP calls.
    """

    name = db.Column(db.String(80), nullable=False)
    url = db.Column(db.String(400))
    entity_type = db.Column(db.String(40), default="all")
    is_ajax = db.Column(db.Boolean(), default=False)

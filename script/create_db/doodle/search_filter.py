from sqlalchemy_utils import UUIDType

from sqlalchemy import orm
import sqlalchemy
from doodle.base import BaseMixin
from sqlalchemy.sql import expression


class SearchFilter(BaseMixin):
    """
    Filters allow to store quick search on a list: asset list, shot list,
    sequence list, todo-list...
    """

    list_type = db.Column(db.String(80), nullable=False, index=True)
    entity_type = db.Column(db.String(80))
    name = db.Column(db.String(200), nullable=False, default="")
    search_query = db.Column(db.String(500), nullable=False, default="")
    is_shared = db.Column(
        db.Boolean,
        default=False,
        nullable=False,
    )

    department_id = db.Column(
        UUIDType(binary=True),
        db.ForeignKey("department.id"),
    )
    search_filter_group_id = db.Column(
        UUIDType(binary=True),
        db.ForeignKey("search_filter_group.id"),
        nullable=True,
    )
    person_id = db.Column(
        UUIDType(binary=True), db.ForeignKey("person.id"), index=True
    )
    project_id = db.Column(
        UUIDType(binary=True), db.ForeignKey("project.id"), index=True
    )

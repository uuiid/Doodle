from sqlalchemy_utils import UUIDType

from sqlalchemy import orm
import sqlalchemy
from doodle.base import BaseMixin


class Subscription(BaseMixin):
    """
    Allow to subscribe to an entity
    """

    person_id = db.Column(
        UUIDType(binary=True),
        db.ForeignKey("person.id"),
        nullable=False,
        index=True,
    )
    task_id = db.Column(
        UUIDType(binary=True), db.ForeignKey("task.id"), index=True
    )

    entity_id = db.Column(
        UUIDType(binary=True), db.ForeignKey("entity.id"), index=True
    )  # Deprecated
    task_type_id = db.Column(
        UUIDType(binary=True), db.ForeignKey("task_type.id"), index=True
    )  # Deprecated

    __table_args__ = (
        db.UniqueConstraint(
            "person_id", "task_id", name="subscription_task_uc"
        ),
        db.UniqueConstraint(
            "person_id",
            "task_type_id",
            "entity_id",
            name="subscription_entity_uc",
        ),
    )

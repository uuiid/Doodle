from sqlalchemy_utils import UUIDType

from sqlalchemy import orm
import sqlalchemy
from doodle.base import BaseMixin
from zou.app.models.subscription import Subscription as ZouSubscription

class Subscription(BaseMixin):
    """
    Allow to subscribe to an entity
    """
    __tablename__ = "subscription"
    uuid: orm.Mapped[UUIDType] = orm.mapped_column(
        UUIDType(binary=True), unique=True, nullable=False, index=True
    )
    person_id = orm.mapped_column(
        UUIDType(binary=True),
        sqlalchemy.ForeignKey("person.uuid"),
        nullable=False,
        index=True,
    )
    task_id = orm.mapped_column(
        UUIDType(binary=True), sqlalchemy.ForeignKey("task.uuid"), index=True
    )

    entity_id = orm.mapped_column(
        UUIDType(binary=True), sqlalchemy.ForeignKey("entity.uuid"), index=True
    )  # Deprecated
    task_type_id = orm.mapped_column(
        UUIDType(binary=True), sqlalchemy.ForeignKey("task_type.uuid"), index=True
    )  # Deprecated

    __table_args__ = (
        sqlalchemy.UniqueConstraint(
            "person_id", "task_id", name="subscription_task_uc"
        ),
        sqlalchemy.UniqueConstraint(
            "person_id",
            "task_type_id",
            "entity_id",
            name="subscription_entity_uc",
        ),
    )

    def from_zou(self, subscription: ZouSubscription):
        self.uuid = subscription.id
        self.person_id = subscription.person_id
        self.task_id = subscription.task_id
        self.entity_id = subscription.entity_id
        self.task_type_id = subscription.task_type_id
        return self
from sqlalchemy_utils import UUIDType, ChoiceType


from sqlalchemy import orm
import sqlalchemy

from doodle.base import BaseMixin
from zou.app.models.notification import Notification as ZouNotification
TYPES = [
    ("comment", "Comment"),
    ("mention", "Mention"),
    ("assignation", "Assignation"),
    ("reply", "Reply"),
    ("reply-mention", "Reply Mention"),
]


class Notification(BaseMixin):
    """
    A notification is stored each time a comment is posted.
    """
    __tablename__ = "notification"

    uuid : orm.Mapped[UUIDType] = orm.mapped_column(
        UUIDType(binary=True), unique=True, nullable=False, index=True
    )
    read = orm.mapped_column(sqlalchemy.Boolean, nullable=False, default=False)
    change = orm.mapped_column(sqlalchemy.Boolean, nullable=False, default=False)
    type = orm.mapped_column(ChoiceType(TYPES), nullable=False)
    person_id = orm.mapped_column(
        UUIDType(binary=True),
        sqlalchemy.ForeignKey("person.uuid"),
        nullable=False,
        index=True,
    )
    author_id = orm.mapped_column(
        UUIDType(binary=True),
        sqlalchemy.ForeignKey("person.uuid"),
        nullable=False,
        index=True,
    )
    comment_id = orm.mapped_column(
        UUIDType(binary=True),
        sqlalchemy.ForeignKey("comment.uuid"),
        nullable=True,
        index=True,
    )
    task_id = orm.mapped_column(
        UUIDType(binary=True),
        sqlalchemy.ForeignKey("task.uuid"),
        nullable=False,
        index=True,
    )
    reply_id = orm.mapped_column(UUIDType(binary=True), nullable=True, index=True)

    __table_args__ = (
        sqlalchemy.UniqueConstraint(
            "person_id",
            "author_id",
            "comment_id",
            "reply_id",
            "type",
            name="notification_uc",
        ),
    )

    def from_zou(self, notification: ZouNotification):
        self.uuid = notification.id
        self.read = notification.read
        self.change = notification.change
        self.type = notification.type
        self.person_id = notification.person_id
        self.author_id = notification.author_id
        self.comment_id = notification.comment_id
        self.task_id = notification.task_id
        self.reply_id = notification.reply_id

        return self
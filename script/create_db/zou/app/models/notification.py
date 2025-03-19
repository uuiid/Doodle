from sqlalchemy_utils import UUIDType, ChoiceType

from sqlalchemy.inspection import inspect

from zou.app import db
from zou.app.models.serializer import SerializerMixin
from zou.app.utils.fields import serialize_value
from zou.app.models.base import BaseMixin

TYPES = [
    ("comment", "Comment"),
    ("mention", "Mention"),
    ("assignation", "Assignation"),
    ("reply", "Reply"),
    ("reply-mention", "Reply Mention"),
]


class Notification(db.Model, BaseMixin, SerializerMixin):
    """
    A notification is stored each time a comment is posted.
    """

    read = db.Column(db.Boolean, nullable=False, default=False)
    change = db.Column(db.Boolean, nullable=False, default=False)
    type = db.Column(ChoiceType(TYPES), nullable=False)
    person_id = db.Column(
        UUIDType(binary=False),
        db.ForeignKey("person.id"),
        nullable=False,
        index=True,
    )
    author_id = db.Column(
        UUIDType(binary=False),
        db.ForeignKey("person.id"),
        nullable=False,
        index=True,
    )
    comment_id = db.Column(
        UUIDType(binary=False),
        db.ForeignKey("comment.id"),
        nullable=True,
        index=True,
    )
    task_id = db.Column(
        UUIDType(binary=False),
        db.ForeignKey("task.id"),
        nullable=False,
        index=True,
    )
    reply_id = db.Column(UUIDType(binary=False), nullable=True, index=True)

    __table_args__ = (
        db.UniqueConstraint(
            "person_id",
            "author_id",
            "comment_id",
            "reply_id",
            "type",
            name="notification_uc",
        ),
    )

    def serialize(self, obj_type=None, relations=False, milliseconds=False):
        attrs = inspect(self).attrs.keys()
        obj_dict = {
            attr: serialize_value(
                getattr(self, attr), milliseconds=milliseconds
            )
            for attr in attrs
        }
        obj_dict["notification_type"] = obj_dict["type"]
        obj_dict["type"] = obj_type or type(self).__name__
        return obj_dict

    @classmethod
    def create_from_import(cls, data):
        notification_type = ""
        if "notification_type" in data:
            notification_type = data.get("notification_type", "")
            del data["notification_type"]
        data["type"] = notification_type
        previous_data = cls.get(data["id"])
        if previous_data is None:
            return (cls.create(**data), False)
        else:
            previous_data.update(data)
            return (previous_data, True)

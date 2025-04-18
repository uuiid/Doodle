from sqlalchemy_utils import UUIDType

from sqlalchemy import orm
import sqlalchemy
from doodle.base import BaseMixin


class News(BaseMixin):
    """
    A news is created each time a comment is posted.
    """

    change = db.Column(db.Boolean, nullable=False, default=False)
    author_id = db.Column(
        UUIDType(binary=True),
        db.ForeignKey("person.id"),
        nullable=False,
        index=True,
    )
    comment_id = db.Column(
        UUIDType(binary=True),
        db.ForeignKey("comment.id"),
        nullable=True,
        index=True,
    )
    task_id = db.Column(
        UUIDType(binary=True),
        db.ForeignKey("task.id"),
        nullable=False,
        index=True,
    )
    preview_file_id = db.Column(
        UUIDType(binary=True),
        db.ForeignKey("preview_file.id"),
        nullable=True,
        index=True,
    )

    @classmethod
    def create_from_import(cls, data):
        data = {
            "id": data["id"],
            "updated_at": data["created_at"],
            "created_at": data["created_at"],
            "change": data["change"],
            "author_id": data["author_id"],
            "comment_id": data["comment_id"],
            "preview_file_id": data["preview_file_id"],
            "task_id": data["task_id"],
        }
        previous_data = cls.get(data["id"])
        if previous_data is None:
            return (cls.create(**data), True)
        else:
            previous_data.update(data)
            return (previous_data, False)

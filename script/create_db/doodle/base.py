from sqlalchemy_utils import UUIDType
from sqlalchemy import func
from sqlalchemy import orm
from sqlalchemy.orm import DeclarativeBase


# class BaseLink(DeclarativeBase):
#     id : orm.Mapped[int]  = orm.mapped_column(primary_key=True, autoincrement=True)

class BaseMixin(DeclarativeBase):
    id: orm.Mapped[int] = orm.mapped_column(primary_key=True, autoincrement=True)
    # uuid_id : orm.Mapped[UUIDType] = orm.mapped_column(
    #     UUIDType(binary=True), unique=True
    # )
    # id = db.Column(
    #     UUIDType(binary=True), primary_key=True, default=fields.gen_uuid
    # )

    # Audit fields
    # created_at = db.Column(
    #     db.DateTime, default=date_helpers.get_utc_now_datetime
    # )
    # updated_at = db.Column(
    #     db.DateTime,
    #     default=date_helpers.get_utc_now_datetime,
    #     onupdate=date_helpers.get_utc_now_datetime,
    # )

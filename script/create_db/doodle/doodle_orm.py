from sqlalchemy_utils import UUIDType, ChoiceType

from sqlalchemy.dialects.postgresql import JSONB

from sqlalchemy import orm
import sqlalchemy

from doodle.base import BaseMixin


class ProjectOld(BaseMixin):
    __tablename__ = "project_tab"
    uuid_id: orm.Mapped[UUIDType] = orm.mapped_column(
        UUIDType(binary=True), unique=True, nullable=False, index=True
    )
    name = orm.mapped_column(sqlalchemy.String(80), nullable=True, unique=True, index=True)
    path = orm.mapped_column(sqlalchemy.String(80))
    en_str = orm.mapped_column(sqlalchemy.String(80))
    auto_upload_path = orm.mapped_column(sqlalchemy.String(80))
    code = orm.mapped_column(sqlalchemy.String(80))


class PersonOld(BaseMixin):
    __tablename__ = "user_tab"
    uuid_id: orm.Mapped[UUIDType] = orm.mapped_column(
        UUIDType(binary=True), unique=True, nullable=False, index=True
    )
    mobile: orm.Mapped[str] = orm.mapped_column(sqlalchemy.String(80))
    dingding_id: orm.Mapped[str] = orm.mapped_column(sqlalchemy.String(80))
    dingding_company_id: orm.Mapped[UUIDType] = orm.mapped_column(
        UUIDType(binary=True)
    )


# class WorkXlsxTaskInfo(BaseMixin):
#     __tablename__ = "work_xlsx_task_info_tab"
#     uuid: orm.Mapped[UUIDType] = orm.mapped_column(
#         UUIDType(binary=True), unique=True, nullable=False, index=True
#     )
#     start_time: orm.Mapped[sqlalchemy.DateTime] = orm.mapped_column(sqlalchemy.DateTime)
#     end_time: orm.Mapped[sqlalchemy.DateTime] = orm.mapped_column(sqlalchemy.DateTime)
#     duration: orm.Mapped[int] = orm.mapped_column(sqlalchemy.Integer)
#     remark = orm.mapped_column(sqlalchemy.String)
#     user_remark = orm.mapped_column(sqlalchemy.String)
#     year_month = orm.mapped_column(sqlalchemy.Integer)
#     user_id = orm.mapped_column(sqlalchemy.DateTime)
#     kitsu_task_ref_id = orm.mapped_column(sqlalchemy.DateTime)
#     season = orm.mapped_column(sqlalchemy.DateTime)
#     episode = orm.mapped_column(sqlalchemy.DateTime)
#     name = orm.mapped_column(sqlalchemy.DateTime)
#     grade = orm.mapped_column(sqlalchemy.DateTime)
#     project_id = orm.mapped_column(sqlalchemy.DateTime)
#     project_name = orm.mapped_column(sqlalchemy.DateTime)

class AssetsTab(BaseMixin):
    __tablename__ = "assets_tab"
    uuid_id: orm.Mapped[UUIDType] = orm.mapped_column(
        UUIDType(binary=True), unique=True, nullable=False, index=True
    )
    label = orm.mapped_column(sqlalchemy.String(80), nullable=False, index=True)
    parent_uuid = orm.mapped_column(UUIDType(binary=True))
    order = orm.mapped_column(sqlalchemy.Integer, server_default=sqlalchemy.text("0"), nullable=False)
    
    def form_old(self, assets):
        self.uuid_id = assets.uuid_id
        self.label = assets.label
        self.parent_uuid = assets.parent_uuid
        self.order = assets.order
        return self


class AssetsFileTab(BaseMixin):
    __tablename__ = "assets_file_tab_2"
    uuid_id: orm.Mapped[UUIDType] = orm.mapped_column(
        UUIDType(binary=True), unique=True, nullable=False, index=True
    )
    label = orm.mapped_column(sqlalchemy.String(80), nullable=True, index=True)
    path = orm.mapped_column(sqlalchemy.String(80))
    notes = orm.mapped_column(sqlalchemy.String)
    active = orm.mapped_column(sqlalchemy.Boolean, nullable=False)
    has_thumbnail = orm.mapped_column(sqlalchemy.Boolean, server_default=sqlalchemy.text("0"), nullable=False)
    extension = orm.mapped_column(sqlalchemy.String(), server_default=".png")

    def form_old(self, assets):
        self.uuid_id = assets.uuid_id
        self.label = assets.label
        self.path = assets.path
        self.notes = assets.notes
        self.active = assets.active
        self.has_thumbnail = assets.has_thumbnail
        self.extension = assets.extension
        return self


class AssetsLinkParent(BaseMixin):
    __tablename__ = "assets_link_parent_t"
    assets_type_uuid = orm.mapped_column(UUIDType(binary=True), sqlalchemy.ForeignKey("assets_tab.uuid_id"), nullable=False)
    assets_uuid = orm.mapped_column(UUIDType(binary=True), sqlalchemy.ForeignKey("assets_file_tab_2.uuid_id"), nullable=False)

    def form_old(self, assets):
        self.assets_type_uuid = assets.assets_type_uuid
        self.assets_uuid = assets.assets_uuid
        return self

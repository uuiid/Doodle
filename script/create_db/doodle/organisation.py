from doodle.base import BaseMixin
from sqlalchemy_utils import UUIDType
from sqlalchemy import orm
import sqlalchemy
from zou.app.models.organisation import Organisation as ZouOrganisation


class Organisation(BaseMixin):
    """
    Model to represent current organisation settings.
    """
    __tablename__ = "organisation"
    uuid: orm.Mapped[UUIDType] = orm.mapped_column(
        UUIDType(binary=True), unique=True, nullable=False, index=True
    )
    name = orm.mapped_column(sqlalchemy.String(80), unique=True, nullable=False)
    hours_by_day = orm.mapped_column(sqlalchemy.Float, default=8, nullable=False)
    has_avatar = orm.mapped_column(sqlalchemy.Boolean(), default=False)
    use_original_file_name = orm.mapped_column(sqlalchemy.Boolean(), default=False)
    timesheets_locked = orm.mapped_column(sqlalchemy.Boolean(), default=False)
    format_duration_in_hours = orm.mapped_column(sqlalchemy.Boolean(), default=False)
    hd_by_default = orm.mapped_column(sqlalchemy.Boolean(), default=False)
    chat_token_slack = orm.mapped_column(sqlalchemy.String(80), default="")
    chat_webhook_mattermost = orm.mapped_column(sqlalchemy.String(80), default="")
    chat_token_discord = orm.mapped_column(sqlalchemy.String(80), default="")
    dark_theme_by_default = orm.mapped_column(sqlalchemy.Boolean(), default=False)
    format_duration_in_hours = orm.mapped_column(sqlalchemy.Boolean(), default=False)

    def from_zou(self, organisation: ZouOrganisation):
        self.name = organisation.name
        self.hours_by_day = organisation.hours_by_day
        self.has_avatar = organisation.has_avatar
        self.use_original_file_name = organisation.use_original_file_name
        self.timesheets_locked = organisation.timesheets_locked
        self.format_duration_in_hours = organisation.format_duration_in_hours
        self.hd_by_default = organisation.hd_by_default
        self.chat_token_slack = organisation.chat_token_slack
        self.chat_webhook_mattermost = organisation.chat_webhook_mattermost
        self.chat_token_discord = organisation.chat_token_discord
        self.dark_theme_by_default = organisation.dark_theme_by_default
        self.format_duration_in_hours = organisation.format_duration_in_hours
        self.uuid = organisation.id
        return self

from sqlalchemy_utils import (
    UUIDType,
    EmailType,
    LocaleType,
    TimezoneType,
    ChoiceType,
)
from sqlalchemy import Index
from sqlalchemy.ext.hybrid import hybrid_property
from sqlalchemy.dialects.postgresql import JSONB

from pytz import timezone as pytz_timezone
from babel import Locale
from sqlalchemy import orm
import sqlalchemy

from doodle.base import BaseMixin
from zou.app.models.person import Person as ZouPerson
from zou.app.models.person import DepartmentLink as ZouDepartmentLink

from doodle.doodle_orm import PersonOld

TWO_FACTOR_AUTHENTICATION_TYPES = [
    ("totp", "TOTP"),
    ("email_otp", "Email OTP"),
    ("fido", "FIDO"),
]

CONTRACT_TYPES = [
    ("open_ended", "Open-ended"),
    ("fixed_term", "Fixed-term"),
    ("short_term", "Short-term"),
    ("freelance", "Freelance"),
    ("apprentice", "Apprentice"),
    ("internship", "Internship"),
]

ROLE_TYPES = [
    ("user", "Artist"),
    ("admin", "Studio Manager"),
    ("supervisor", "Supervisor"),
    ("manager", "Production Manager"),
    ("client", "Client"),
    ("vendor", "Vendor"),
]


class DepartmentLink(BaseMixin):
    __tablename__ = "department_link"
    person_id = orm.mapped_column(
        UUIDType(binary=True),
        sqlalchemy.ForeignKey("person.uuid"),
        index=True,
    )
    department_id = orm.mapped_column(
        UUIDType(binary=True),
        sqlalchemy.ForeignKey("department.uuid"),
        index=True,
    )

    __table_args__ = (
        sqlalchemy.UniqueConstraint(
            "person_id",
            "department_id",
            name="department_link_uc",
        ),
    )

    def from_zou(self, department_link: ZouDepartmentLink):
        self.person_id = department_link.person_id
        self.department_id = department_link.department_id

        return self


class Person(BaseMixin):
    """
    Describe a member of the studio (and an API user).
    """
    __tablename__ = "person"
    uuid: orm.Mapped[UUIDType] = orm.mapped_column(
        UUIDType(binary=True), unique=True, nullable=False, index=True
    )
    first_name = orm.mapped_column(sqlalchemy.String(80), nullable=True)
    last_name = orm.mapped_column(sqlalchemy.String(80), nullable=True)
    email = orm.mapped_column(EmailType)
    phone = orm.mapped_column(sqlalchemy.String(30))
    contract_type = orm.mapped_column(
        ChoiceType(CONTRACT_TYPES), default="open-ended", nullable=False
    )

    active = orm.mapped_column(sqlalchemy.Boolean(), default=True, nullable=False)
    archived = orm.mapped_column(sqlalchemy.Boolean(), default=False, nullable=False)
    last_presence = orm.mapped_column(sqlalchemy.Date())

    password = orm.mapped_column(sqlalchemy.LargeBinary(60))
    desktop_login = orm.mapped_column(sqlalchemy.String(80))
    login_failed_attemps = orm.mapped_column(sqlalchemy.Integer, default=0, nullable=False)
    last_login_failed = orm.mapped_column(sqlalchemy.DateTime())
    totp_enabled = orm.mapped_column(sqlalchemy.Boolean(), default=False, nullable=False)
    totp_secret = orm.mapped_column(sqlalchemy.String(32), default=None)
    email_otp_enabled = orm.mapped_column(sqlalchemy.Boolean(), default=False, nullable=False)
    email_otp_secret = orm.mapped_column(sqlalchemy.String(32), default=None)
    fido_enabled = orm.mapped_column(sqlalchemy.Boolean(), default=False, nullable=False)
    fido_credentials = orm.mapped_column(sqlalchemy.TEXT())
    otp_recovery_codes = orm.mapped_column(sqlalchemy.TEXT())
    preferred_two_factor_authentication = orm.mapped_column(
        ChoiceType(TWO_FACTOR_AUTHENTICATION_TYPES)
    )

    shotgun_id = orm.mapped_column(sqlalchemy.Integer, unique=True)
    timezone = orm.mapped_column(
        TimezoneType(backend="pytz"),
    )
    locale = orm.mapped_column(LocaleType)
    data = orm.mapped_column(sqlalchemy.TEXT())
    role = orm.mapped_column(ChoiceType(ROLE_TYPES), default="user", nullable=False)
    has_avatar = orm.mapped_column(sqlalchemy.Boolean(), default=False, nullable=False)

    notifications_enabled = orm.mapped_column(sqlalchemy.Boolean(), default=False, nullable=False)
    notifications_slack_enabled = orm.mapped_column(sqlalchemy.Boolean(), default=False, nullable=False)
    notifications_slack_userid = orm.mapped_column(sqlalchemy.String(60), default="")
    notifications_mattermost_enabled = orm.mapped_column(sqlalchemy.Boolean(), default=False, nullable=False)
    notifications_mattermost_userid = orm.mapped_column(sqlalchemy.String(60), default="")
    notifications_discord_enabled = orm.mapped_column(sqlalchemy.Boolean(), default=False, nullable=False)
    notifications_discord_userid = orm.mapped_column(sqlalchemy.String(60), default="")

    is_bot = orm.mapped_column(sqlalchemy.Boolean(), default=False, nullable=False)
    jti = orm.mapped_column(sqlalchemy.String(60), nullable=True, unique=True)
    expiration_date = orm.mapped_column(sqlalchemy.Date())

    departments = orm.relationship(
        "Department", secondary="department_link", lazy="joined"
    )
    studio_id = orm.mapped_column(
        UUIDType(binary=True), sqlalchemy.ForeignKey("studio.uuid"), index=True
    )

    is_generated_from_ldap = orm.mapped_column(sqlalchemy.Boolean(), default=False, nullable=False)
    ldap_uid = orm.mapped_column(sqlalchemy.String(60), unique=True, default=None)

    dingding_id: orm.Mapped[str] = orm.mapped_column(sqlalchemy.String(80), nullable=True)
    dingding_company_id: orm.Mapped[UUIDType] = orm.mapped_column(
        UUIDType(binary=True), nullable=True
    )

    __table_args__ = (
        Index(
            "only_one_email_by_person",
            email,
            is_bot,
            unique=True,
            postgresql_where=is_bot.isnot(True),
        ),
    )

    def from_zou(self, person: ZouPerson):
        self.uuid = person.id
        self.first_name = person.first_name
        self.last_name = person.last_name
        self.email = person.email
        self.phone = person.phone
        self.contract_type = person.contract_type
        self.contract_type.code = self.contract_type.code.replace("-", "_")
        self.active = person.active
        self.archived = person.archived
        self.last_presence = person.last_presence
        self.password = person.password
        self.desktop_login = person.desktop_login
        self.login_failed_attemps = person.login_failed_attemps
        self.last_login_failed = person.last_login_failed
        self.totp_enabled = person.totp_enabled
        self.totp_secret = person.totp_secret
        self.email_otp_enabled = person.email_otp_enabled
        self.email_otp_secret = person.email_otp_secret
        self.fido_enabled = person.fido_enabled
        self.fido_credentials = person.fido_credentials
        self.otp_recovery_codes = ""  # person.otp_recovery_codes
        self.preferred_two_factor_authentication = person.preferred_two_factor_authentication
        self.shotgun_id = person.shotgun_id
        self.timezone = person.timezone
        self.locale = person.locale
        self.data = person.data
        self.role = person.role
        self.has_avatar = person.has_avatar
        self.notifications_enabled = person.notifications_enabled
        self.notifications_slack_enabled = person.notifications_slack_enabled
        self.notifications_slack_userid = person.notifications_slack_userid
        self.notifications_mattermost_enabled = person.notifications_mattermost_enabled
        self.notifications_mattermost_userid = person.notifications_mattermost_userid
        self.notifications_discord_enabled = person.notifications_discord_enabled
        self.notifications_discord_userid = person.notifications_discord_userid
        self.is_bot = person.is_bot
        self.jti = person.jti
        self.expiration_date = person.expiration_date
        self.studio_id = person.studio_id
        self.is_generated_from_ldap = person.is_generated_from_ldap
        self.ldap_uid = person.ldap_uid

        return self

    def form_old(self, person_map: dict[str, PersonOld]):
        if self.uuid in person_map.keys():
            person: PersonOld = person_map[self.uuid]
            self.dingding_id = person.dingding_id
            self.dingding_company_id = person.dingding_company_id
        return self

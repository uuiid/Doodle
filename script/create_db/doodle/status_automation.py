from doodle.base import BaseMixin
from sqlalchemy_utils import UUIDType, ChoiceType

from sqlalchemy import orm
import sqlalchemy

from zou.app.models.status_automation import StatusAutomation as ZouStatusAutomation

CHANGE_TYPES = [("status", "Status"), ("ready_for", "Ready for")]


class StatusAutomation(BaseMixin):
    """
    Status automations are entries that allow to describe changes that
    should automatically apply after a task status is changed.

    For instance, a Modeling task set to done will imply to set the Rigging
    task status to ready and the *ready_for* field to be set at Layout.
    """
    __tablename__ = "status_automation"

    uuid_id: orm.Mapped[UUIDType] = orm.mapped_column(
        UUIDType(binary=True), unique=True, nullable=False, index=True
    )
    entity_type = orm.mapped_column(sqlalchemy.String(40), default="asset")

    in_task_type_id = orm.mapped_column(
        UUIDType(binary=False), sqlalchemy.ForeignKey("task_type.uuid_id"), index=True
    )
    in_task_status_id = orm.mapped_column(
        UUIDType(binary=False), sqlalchemy.ForeignKey("task_status.uuid_id"), index=True
    )

    out_field_type = orm.mapped_column(
        ChoiceType(CHANGE_TYPES), default="status", nullable=False
    )
    out_task_type_id = orm.mapped_column(
        UUIDType(binary=False), sqlalchemy.ForeignKey("task_type.uuid_id"), index=True
    )
    out_task_status_id = orm.mapped_column(
        UUIDType(binary=False),
        sqlalchemy.ForeignKey("task_status.uuid_id"),
        index=True,
        nullable=True,
    )
    import_last_revision = orm.mapped_column(sqlalchemy.Boolean(), default=False)

    archived = orm.mapped_column(sqlalchemy.Boolean(), default=False)

    def from_zou(self, status_automation: ZouStatusAutomation):
        self.entity_type = status_automation.entity_type
        self.in_task_type_id = status_automation.in_task_type_id
        self.in_task_status_id = status_automation.in_task_status_id
        self.out_field_type = status_automation.out_field_type
        self.out_task_type_id = status_automation.out_task_type_id
        self.out_task_status_id = status_automation.out_task_status_id
        self.import_last_revision = status_automation.import_last_revision
        self.archived = status_automation.archived
        self.uuid_id = status_automation.id
        return self
